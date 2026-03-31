package graphics

import "vendor:wgpu"

create_raw_texture :: proc(
    gpu: ^Context,
    data: ^[]byte,
    width: u32,
    height: u32,
    format: wgpu.TextureFormat = .BGRA8UnormSrgb,
    dimension: wgpu.TextureDimension = ._2D,
    usage: wgpu.TextureUsageFlags = {.CopyDst},
) {
    texture := wgpu.DeviceCreateTexture(
        gpu.device,
        &{
            label = "FrameTexture",
            size = {width = width, height = height, depthOrArrayLayers = 1},
            mipLevelCount = 1,
            sampleCount = 1,
            dimension = ._2D,
            format = format,
            usage = usage,
        },
    )

    context_enqueue_write_texture(
        gpu,
        &{
            texture = texture,
            mipLevel = 0,
            origin = {x = 0, y = 0, z = 0},
            aspect = .All,
        },
        data,
        &{offset = 0, bytesPerRow = 4 * width, rowsPerImage = 0},
        &{width = width, height = height, depthOrArrayLayers = 1},
    )
}

