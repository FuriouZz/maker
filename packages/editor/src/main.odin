package maker

import "./graphics"
import "./renderer"
import "base:runtime"
import "core:fmt"
import "core:time"
import "vendor:glfw"
import "vendor:wgpu"
import "vendor:wgpu/glfwglue"

PROFILE :: #config(PROFILE, "debug")
VERBOSE :: #config(VERBOSE, false)

state: struct {
    ctx:      runtime.Context,
    window:   glfw.WindowHandle,

    // wgpu state
    gpu:      graphics.Context,
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
    state.window = glfw.CreateWindow(
        960,
        540,
        "WGPU Native Triangle",
        nil,
        nil,
    )
    defer glfw.DestroyWindow(state.window)

    glfw.SetFramebufferSizeCallback(state.window, on_resize)


    init_wgpu()
    defer uninit_wgpu()
}

init_wgpu :: proc() {
    instance := wgpu.CreateInstance(nil)
    if instance == nil {
        panic("[wgpu] not supported")
    }

    surface := glfwglue.GetSurface(instance, state.window)

    gpu := &state.gpu
    graphics.context_init(gpu, instance, surface, ready)

    ready :: proc(gpu: ^graphics.Context) {
        state.pipeline = renderer.create_hello_world(gpu.device)

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
    graphics.context_uninit(&state.gpu)
}

get_framebuffer_size :: proc() -> (width, height: u32) {
    w, h := glfw.GetFramebufferSize(state.window)
    return u32(w), u32(h)
}

resize :: proc() {
    width, height := get_framebuffer_size()
    graphics.context_resize_surface(&state.gpu, width, height)
}

on_resize :: proc "c" (window: glfw.WindowHandle, width: i32, height: i32) {
    context = state.ctx
    resize()
}

on_frame :: proc "c" (dt: f32) {
    context = state.ctx
    gpu := &state.gpu

    surface_texture := wgpu.SurfaceGetCurrentTexture(gpu.surface)
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
        fmt.panicf(
            "[triangle] get_current_texture status=%v",
            surface_texture.status,
        )
    }
    defer wgpu.TextureRelease(surface_texture.texture)

    frame := wgpu.TextureCreateView(surface_texture.texture, nil)
    defer wgpu.TextureViewRelease(frame)

    encoder := wgpu.DeviceCreateCommandEncoder(gpu.device, nil)
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

    wgpu.QueueSubmit(gpu.queue, {command_buffer})
    wgpu.SurfacePresent(gpu.surface)
}

