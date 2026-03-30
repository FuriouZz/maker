package maker

import "base:runtime"
import "core:fmt"
import "core:time"
import "vendor:glfw"
import "vendor:wgpu"
import "vendor:wgpu/glfwglue"

import "./renderer"

PROFILE :: #config(PROFILE, "debug")
VERBOSE :: #config(VERBOSE, false)

state: struct {
	ctx:      runtime.Context,
	window:   glfw.WindowHandle,

	// wgpu state
	instance: wgpu.Instance,
	surface:  wgpu.Surface,
	config:   wgpu.SurfaceConfiguration,
	adapter:  wgpu.Adapter,
	device:   wgpu.Device,
	queue:    wgpu.Queue,
	pipeline: renderer.HelloWorldPipeline,
}

main :: proc() {
	state.ctx = context

	fmt.printfln("PROFILE=%s VERBOSE=%t", PROFILE, VERBOSE)
	fmt.println("Hellope!")

	if !glfw.Init() {
		panic("[glfw] init failure")
	}
	defer glfw.Terminate()

	glfw.WindowHint(glfw.CLIENT_API, glfw.NO_API)
	state.window = glfw.CreateWindow(960, 540, "WGPU Native Triangle", nil, nil)
	defer glfw.DestroyWindow(state.window)

	glfw.SetFramebufferSizeCallback(state.window, on_resize)

	init_wgpu()
	defer uninit_wgpu()
}

init_wgpu :: proc() {
	state.instance = wgpu.CreateInstance(nil)
	if state.instance == nil {
		panic("[wgpu] not supported")
	}

	state.surface = glfwglue.GetSurface(state.instance, state.window)

	wgpu.InstanceRequestAdapter(
		state.instance,
		&{compatibleSurface = state.surface},
		{callback = on_adapter},
	)

	on_adapter :: proc "c" (
		status: wgpu.RequestAdapterStatus,
		adapter: wgpu.Adapter,
		message: string,
		userdata1: rawptr,
		userdata2: rawptr,
	) {
		context = state.ctx
		if status != .Success || adapter == nil {
			fmt.panicf("request adapter failure: [%v] %s", status, message)
		}
		state.adapter = adapter
		wgpu.AdapterRequestDevice(adapter, nil, {callback = on_device})
	}

	on_device :: proc "c" (
		status: wgpu.RequestDeviceStatus,
		device: wgpu.Device,
		message: wgpu.StringView,
		userdata1: rawptr,
		userdata2: rawptr,
	) {
		context = state.ctx
		if status != .Success || device == nil {
			fmt.panicf("request device failure: [%v] %s", status, message)
		}
		state.device = device

		width, height := get_framebuffer_size()

		state.config = wgpu.SurfaceConfiguration {
			width       = width,
			height      = height,
			device      = state.device,
			usage       = {.RenderAttachment},
			format      = .BGRA8Unorm,
			presentMode = .Fifo,
			alphaMode   = .Opaque,
		}

		wgpu.SurfaceConfigure(state.surface, &state.config)
		state.queue = wgpu.DeviceGetQueue(state.device)

		state.pipeline = renderer.create_hello_world(state.device)

		dt: f32
		for !glfw.WindowShouldClose(state.window) {
			start := time.tick_now()

			glfw.PollEvents()
			on_frame(dt)

			dt = f32(time.duration_seconds(time.tick_since(start)))
		}
	}
}

uninit_wgpu :: proc() {
	renderer.release_hello_world(state.pipeline)
	wgpu.QueueRelease(state.queue)
	wgpu.DeviceRelease(state.device)
	wgpu.AdapterRelease(state.adapter)
	wgpu.SurfaceRelease(state.surface)
	wgpu.InstanceRelease(state.instance)
}


get_framebuffer_size :: proc() -> (width, height: u32) {
	w, h := glfw.GetFramebufferSize(state.window)
	return u32(w), u32(h)
}

resize :: proc "c" () {
	context = state.ctx
	state.config.width, state.config.height = get_framebuffer_size()
	wgpu.SurfaceConfigure(state.surface, &state.config)
}

on_resize :: proc "c" (window: glfw.WindowHandle, width: i32, height: i32) {
	context = state.ctx
	resize()
}

on_frame :: proc "c" (dt: f32) {
	context = state.ctx

	surface_texture := wgpu.SurfaceGetCurrentTexture(state.surface)
	switch surface_texture.status {
	case .SuccessOptimal, .SuccessSuboptimal:
	// All good, could handle suboptimal here.
	case .Timeout, .Outdated, .Lost:
		// Skip this frame, and re-configure surface.
		if surface_texture.texture != nil {
			wgpu.TextureRelease(surface_texture.texture)
		}
		resize()
		return
	case .OutOfMemory, .DeviceLost, .Error:
		// Fatal error
		fmt.panicf("[triangle] get_current_texture status=%v", surface_texture.status)
	}
	defer wgpu.TextureRelease(surface_texture.texture)

	frame := wgpu.TextureCreateView(surface_texture.texture, nil)
	defer wgpu.TextureViewRelease(frame)

	encoder := wgpu.DeviceCreateCommandEncoder(state.device, nil)
	defer wgpu.CommandEncoderRelease(encoder)

	render_pass_encoder := wgpu.CommandEncoderBeginRenderPass(
		encoder,
		&{
			colorAttachmentCount = 1,
			colorAttachments = &wgpu.RenderPassColorAttachment {
				view = frame,
				loadOp = .Clear,
				storeOp = .Store,
				depthSlice = wgpu.DEPTH_SLICE_UNDEFINED,
				clearValue = {0, 1, 0, 1},
			},
		},
	)

	renderer.draw_hello_world(state.pipeline, render_pass_encoder)

	wgpu.RenderPassEncoderEnd(render_pass_encoder)
	wgpu.RenderPassEncoderRelease(render_pass_encoder)

	command_buffer := wgpu.CommandEncoderFinish(encoder, nil)
	defer wgpu.CommandBufferRelease(command_buffer)

	wgpu.QueueSubmit(state.queue, {command_buffer})
	wgpu.SurfacePresent(state.surface)
}

