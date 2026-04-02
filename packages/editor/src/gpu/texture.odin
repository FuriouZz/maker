package gpu

import "vendor:wgpu"

create_texture :: proc(
    self: ^Context,
    desc: ^wgpu.TextureDescriptor,
) -> wgpu.Texture {
    return wgpu.DeviceCreateTexture(self.device, desc)
}

write_texture :: proc(
    self: ^Context,
    src: ^TextureSource,
    dst: ^wgpu.TexelCopyTextureInfo,
    copy_size: ^wgpu.Extent3D,
) {
    switch &s in src^ {
    case wgpu.TexelCopyBufferInfo:
        wgpu.CommandEncoderCopyBufferToTexture(
            self.encoder,
            &s,
            dst,
            copy_size,
        )
    case wgpu.TexelCopyTextureInfo:
        wgpu.CommandEncoderCopyTextureToTexture(
            self.encoder,
            &s,
            dst,
            copy_size,
        )
    }
}

enqueue_write_texture :: proc(
    self: ^Context,
    texture: ^wgpu.TexelCopyTextureInfo,
    data: rawptr,
    dataSize: uint,
    data_layout: ^wgpu.TexelCopyBufferLayout,
    size: ^wgpu.Extent3D,
) {
    wgpu.QueueWriteTexture(
        self.queue,
        texture,
        data,
        dataSize,
        data_layout,
        size,
    )
}

create_raw_texture :: proc(
    self: ^Context,
    data: ^[]byte,
    width: u32,
    height: u32,
    format: wgpu.TextureFormat = PREFERED_TEXTURE_FORMAT,
    dimension: wgpu.TextureDimension = ._2D,
    usage: wgpu.TextureUsageFlags = {.CopyDst},
) -> (
    texture: wgpu.Texture,
) {

    texture = create_texture(
        self,
        &{
            label = "create_raw_texture",
            size = {width = width, height = height, depthOrArrayLayers = 1},
            mipLevelCount = 1,
            sampleCount = 1,
            dimension = dimension,
            format = format,
            usage = usage,
        },
    )

    update_raw_texture(self, texture, data, width, height)

    return texture
}

update_raw_texture :: proc(
    self: ^Context,
    texture: wgpu.Texture,
    data: ^[]byte,
    width: u32,
    height: u32,
) {
    // enqueue_write_texture(
    //     self,
    //     &{
    //         texture = texture,
    //         mipLevel = 0,
    //         origin = {x = 0, y = 0, z = 0},
    //         aspect = .All,
    //     },
    //     data,
    //     len(data^),
    //     &{offset = 0, bytesPerRow = 4 * width, rowsPerImage = height},
    //     &{width = width, height = height, depthOrArrayLayers = 1},
    // )
}

