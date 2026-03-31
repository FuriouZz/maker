package maker

import app "./application"
import "./renderer"
import "core:fmt"
import "vendor:wgpu"

PROFILE :: #config(PROFILE, "debug")
VERBOSE :: #config(VERBOSE, false)

Application :: struct {
    using _app: app.Application, /* #subtype */
    pipeline:   renderer.HelloWorldPipeline,
}

main :: proc() {
    fmt.printfln("PROFILE=%s VERBOSE=%t", PROFILE, VERBOSE)
    fmt.println("Hellope!")

    app.init(
        Application,
        {
            on_ready = app.OnReadyCallback(on_ready),
            on_frame = app.OnFrameCallback(on_frame),
            on_resize = app.OnResizeCallback(on_resize),
        },
    )
}

on_ready :: proc(self: ^Application) {
    gpu := &self.gpu
    self.pipeline = renderer.create_hello_world(gpu.device)
}

on_resize :: proc(self: ^Application) {
    w, h := app.get_size(self)
    fmt.println(w, h)
}

on_frame :: proc(self: ^Application, dt: f32) {
    gpu := &self.gpu

    surface_texture := wgpu.SurfaceGetCurrentTexture(gpu.surface)
    switch surface_texture.status {
    case .SuccessOptimal, .SuccessSuboptimal:
    // All good, could handle suboptimal here.
    case .Timeout, .Outdated, .Lost:
        // Skip this frame, and re-configure surface.
        if surface_texture.texture != nil {
            wgpu.TextureRelease(surface_texture.texture)
        }
        on_resize(self)
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

    renderer.draw_hello_world(self.pipeline, render_pass_encoder)

    wgpu.RenderPassEncoderEnd(render_pass_encoder)
    wgpu.RenderPassEncoderRelease(render_pass_encoder)

    command_buffer := wgpu.CommandEncoderFinish(encoder, nil)
    defer wgpu.CommandBufferRelease(command_buffer)

    wgpu.QueueSubmit(gpu.queue, {command_buffer})
    wgpu.SurfacePresent(gpu.surface)
}

