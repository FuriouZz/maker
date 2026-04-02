package gpu

import "vendor:wgpu"

COPY_BUFFER_ALIGNMENT :: 4
PREFERED_TEXTURE_FORMAT: wgpu.TextureFormat : .BGRA8Unorm

TextureSource :: union {
    wgpu.TexelCopyBufferInfo,
    wgpu.TexelCopyTextureInfo,
}

