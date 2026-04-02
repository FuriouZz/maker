package maker

import app "./application"
import GPU "./gpu"
import "./gpu/geometry"
import "./gpu/pipeline"
import "./makerc"
import "core:fmt"
import "vendor:wgpu"

PROFILE :: #config(PROFILE, "debug")
VERBOSE :: #config(VERBOSE, false)

Application :: struct {
    using _app:   app.Application,
    hello:        pipeline.HelloWorldPipeline,
    sprite:       pipeline.Sprite,
    quad:         geometry.Geometry,
    decoder:      makerc.Decoder,
    frame:        makerc.VideoFrame,
    texture:      wgpu.Texture,
    texture_view: wgpu.TextureView,
    sampler:      wgpu.Sampler,
    bind_group:   wgpu.BindGroup,
    media_info:   makerc.MediaInfo,
    entries:      []wgpu.BindGroupEntry,
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
    self.hello = pipeline.create_hello_world(gpu.device)
    self.sprite = pipeline.create_sprite(gpu)
    self.quad = geometry.create_quad(gpu)

    makerc.decoder_init(
        &self.decoder,
        &{url = cstring("../makerc/tests/video.mp4"), use_playback = 1},
    )

    makerc.decoder_get_media_info(&self.decoder, &self.media_info)

    makerc.video_frame_init(
        &self.frame,
        &{
            width = self.media_info.video_width,
            height = self.media_info.video_height,
            format = .RGBA,
        },
    )

    self.texture = wgpu.DeviceCreateTexture(
        gpu.device,
        &{
            label = "Video Texture",
            size = {
                width = self.media_info.video_width,
                height = self.media_info.video_height,
                depthOrArrayLayers = 1,
            },
            mipLevelCount = 1,
            sampleCount = 1,
            dimension = ._2D,
            format = .RGBA8Unorm,
            usage = {.CopySrc, .CopyDst, .TextureBinding},
        },
    )

    self.texture_view = wgpu.TextureCreateView(
        self.texture,
        &{
            label = "Video Texture View",
            dimension = ._2D,
            format = .RGBA8Unorm,
            usage = {.CopyDst, .TextureBinding},
            baseArrayLayer = 0,
            arrayLayerCount = 1,
            baseMipLevel = 0,
            mipLevelCount = 1,
            aspect = .Undefined,
        },
    )

    self.sampler = wgpu.DeviceCreateSampler(
        gpu.device,
        &{
            label = "Video Sampler",
            addressModeU = .ClampToEdge,
            addressModeV = .ClampToEdge,
            addressModeW = .ClampToEdge,
            magFilter = .Linear,
            minFilter = .Linear,
            maxAnisotropy = 1,
        },
    )

    bind_group, entries := pipeline.sprite_create_bind_group(
        &self.sprite,
        gpu,
        self.texture_view,
        self.sampler,
    )

    self.entries = entries
    self.bind_group = bind_group
}

on_finish :: proc(self: ^Application) {
    pipeline.release_hello_world(self.hello)
    pipeline.sprite_release(&self.sprite)
    geometry.release(&self.quad)
    makerc.decoder_uninit(&self.decoder)
    makerc.video_frame_uninit(&self.frame)
}

on_resize :: proc(self: ^Application) {
    w, h := app.get_size(self)
    fmt.println(w, h)
}

on_frame :: proc(self: ^Application, dt: f32) {
    gpu := &self.gpu
    decoder := &self.decoder
    target := &self.frame

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

    makerc.decoder_get_video_frame(decoder, target)

    wgpu.QueueWriteTexture(
        gpu.queue,
        &{
            texture = self.texture,
            aspect = .All,
            mipLevel = 0,
            origin = {0, 0, 0},
        },
        target.buffer,
        uint(target.buffer_size),
        &{
            offset = 0,
            bytesPerRow = self.media_info.video_width * 4,
            rowsPerImage = self.media_info.video_height,
        },
        &{
            width = self.media_info.video_width,
            height = self.media_info.video_height,
            depthOrArrayLayers = 1,
        },
    )

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

    // pipeline.draw_hello_world(self.hello, render_pass_encoder)

    pipeline.sprite_draw(
        &self.sprite,
        pass = render_pass_encoder,
        geometry = self.quad,
        bind_group = self.bind_group,
    )

    wgpu.RenderPassEncoderEnd(render_pass_encoder)
    wgpu.RenderPassEncoderRelease(render_pass_encoder)

    command_buffer := wgpu.CommandEncoderFinish(encoder, nil)
    defer wgpu.CommandBufferRelease(command_buffer)

    wgpu.QueueSubmit(gpu.queue, {command_buffer})
    wgpu.SurfacePresent(gpu.surface)
}

