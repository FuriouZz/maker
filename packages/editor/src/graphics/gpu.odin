package graphics

import "base:intrinsics"
import "base:runtime"
import "core:fmt"
import "vendor:wgpu"

Context :: struct {
    instance:       wgpu.Instance,
    surface:        wgpu.Surface,
    surface_config: wgpu.SurfaceConfiguration,
    device:         wgpu.Device,
    encoder:        wgpu.CommandEncoder,
    adapter:        wgpu.Adapter,
    queue:          wgpu.Queue,
}

TextureSource :: union {
    wgpu.TexelCopyBufferInfo,
    wgpu.TexelCopyTextureInfo,
}

Internal :: struct {
    gpu:      ^Context,
    ctx:      runtime.Context,
    ready:    proc(userdata: rawptr),
    userdata: rawptr,
}

context_init :: proc(
    gpu: ^Context,
    instance: wgpu.Instance,
    surface: wgpu.Surface,
    ready: proc(userdata: rawptr),
    userdata: rawptr,
) {
    gpu.instance = instance
    gpu.surface = surface

    internal := Internal {
        gpu      = gpu,
        ctx      = context,
        ready    = ready,
        userdata = userdata,
    }

    wgpu.InstanceRequestAdapter(
        gpu.instance,
        &{compatibleSurface = gpu.surface},
        {callback = on_adapter, userdata1 = &internal},
    )

    on_adapter :: proc "c" (
        status: wgpu.RequestAdapterStatus,
        adapter: wgpu.Adapter,
        message: string,
        userdata1: rawptr,
        userdata2: rawptr,
    ) {
        internal := cast(^Internal)userdata1
        context = internal.ctx
        gpu := internal.gpu

        if status != .Success || adapter == nil {
            fmt.panicf("request adapter failure: [%v] %s", status, message)
        }

        gpu.adapter = adapter
        wgpu.AdapterRequestDevice(
            adapter,
            nil,
            {callback = on_device, userdata1 = internal},
        )
    }

    on_device :: proc "c" (
        status: wgpu.RequestDeviceStatus,
        device: wgpu.Device,
        message: wgpu.StringView,
        userdata1: rawptr,
        userdata2: rawptr,
    ) {
        internal := cast(^Internal)userdata1
        context = internal.ctx
        gpu := internal.gpu

        if status != .Success || device == nil {
            fmt.panicf("request device failure: [%v] %s", status, message)
        }
        gpu.device = device

        gpu.surface_config = wgpu.SurfaceConfiguration {
            width       = 800,
            height      = 600,
            device      = gpu.device,
            usage       = {.RenderAttachment},
            format      = PREFERED_TEXTURE_FORMAT,
            presentMode = .Fifo,
            alphaMode   = .Opaque,
        }

        wgpu.SurfaceConfigure(gpu.surface, &gpu.surface_config)
        gpu.queue = wgpu.DeviceGetQueue(gpu.device)

        if internal.ready != nil {
            internal.ready(internal.userdata)
        }
    }
}

context_uninit :: proc(gpu: ^Context) {
    wgpu.QueueRelease(gpu.queue)
    wgpu.DeviceRelease(gpu.device)
    wgpu.AdapterRelease(gpu.adapter)
    wgpu.SurfaceRelease(gpu.surface)
    wgpu.InstanceRelease(gpu.instance)
}

context_resize_surface :: proc(gpu: ^Context, width: u32, height: u32) {
    gpu.surface_config.width = width
    gpu.surface_config.height = height
    wgpu.SurfaceConfigure(gpu.surface, &gpu.surface_config)
}

context_create_texture :: proc(
    gpu: ^Context,
    desc: ^wgpu.TextureDescriptor,
) -> wgpu.Texture {
    return wgpu.DeviceCreateTexture(gpu.device, desc)
}

context_write_texture :: proc(
    gpu: ^Context,
    src: ^TextureSource,
    dst: ^wgpu.TexelCopyTextureInfo,
    copy_size: ^wgpu.Extent3D,
) {
    switch &s in src^ {
    case wgpu.TexelCopyBufferInfo:
        wgpu.CommandEncoderCopyBufferToTexture(gpu.encoder, &s, dst, copy_size)
    case wgpu.TexelCopyTextureInfo:
        wgpu.CommandEncoderCopyTextureToTexture(
            gpu.encoder,
            &s,
            dst,
            copy_size,
        )
    }
}

context_create_buffer :: proc(
    gpu: ^Context,
    mapped_at_creation: b32,
    usage: wgpu.BufferUsageFlags,
    size: u64,
) -> wgpu.Buffer {
    return wgpu.DeviceCreateBuffer(
        gpu.device,
        &{
            label = "context_create_buffer",
            mappedAtCreation = mapped_at_creation,
            usage = usage,
            size = size,
        },
    )
}

// SOURCE: https://github.com/gfx-rs/wgpu/blob/be1a7114ede120a9faf96f33ad87079be8a30998/wgpu/src/util/device.rs#L40
context_create_buffer_with_data_slice :: proc(
    gpu: ^Context,
    usage: wgpu.BufferUsageFlags,
    data: []$T,
) -> (
    buffer: wgpu.Buffer,
) {
    size := len(data)
    if size == 0 {
        buffer = wgpu.DeviceCreateBuffer(
            gpu.device,
            &{
                label = "context_create_buffer_with_data_slice",
                mappedAtCreation = false,
                size = 0,
                usage = usage,
            },
        )
    } else {
        unpadded_size := size
        // Valid vulkan usage is
        // 1. buffer size must be a multiple of COPY_BUFFER_ALIGNMENT.
        // 2. buffer size must be greater than 0.
        // Therefore we round the value up to the nearest multiple, and ensure it's at least COPY_BUFFER_ALIGNMENT.
        align_mask := COPY_BUFFER_ALIGNMENT - 1
        padded_size := max(
            (size + align_mask) & !align_mask,
            COPY_BUFFER_ALIGNMENT,
        )

        buffer = wgpu.DeviceCreateBuffer(
            gpu.device,
            &{
                label = "context_create_buffer_with_data_slice",
                mappedAtCreation = true,
                size = padded_size,
                usage = usage,
            },
        )

        range := wgpu.BufferGetMappedRangeSlice(buffer, 0, T, padded_size)
        copy(range, data)
        wgpu.BufferUnmap(buffer)
    }

    return buffer
}

context_create_buffer_with_data_typed :: proc(
    gpu: ^Context,
    usage: wgpu.BufferUsageFlags,
    data: $T,
) -> wgpu.Buffer where !intrinsics.type_is_sliceable(T) {
    return context_create_buffer_with_data_slice(gpu, usage, data)
}

context_create_buffer_with_data :: proc {
    context_create_buffer_with_data_slice,
    context_create_buffer_with_data_typed,
}

context_write_buffer_slice :: proc(
    gpu: ^Context,
    dst: wgpu.Buffer,
    data: []$T,
) {
    source := context_create_buffer_with_data_slice(gpu, .CopySrc, data)
    wgpu.CommandEncoderCopyBufferToBuffer(
        gpu.encoder,
        &source,
        0,
        dst,
        0,
        len(data),
    )
}

context_write_buffer_typed :: proc(
    gpu: ^Context,
    dst: wgpu.Buffer,
    data: $T,
) where !intrinsics.type_is_sliceable($T) {
    source := context_create_buffer_with_data_typed(gpu, .CopySrc, data)
    wgpu.CommandEncoderCopyBufferToBuffer(
        gpu.encoder,
        source,
        0,
        dst,
        0,
        len(data),
    )
}

context_write_buffer :: proc {
    context_write_buffer_slice,
    context_write_buffer_typed,
}

context_enqueue_write_buffer :: proc(
    gpu: ^Context,
    buffer: wgpu.Buffer,
    offset: u64,
    data: ^[]byte,
) {
    wgpu.QueueWriteBuffer(gpu.queue, buffer, offset, data, len(data^))
}

context_enqueue_write_texture :: proc(
    gpu: ^Context,
    texture: ^wgpu.TexelCopyTextureInfo,
    data: ^[]byte,
    data_layout: ^wgpu.TexelCopyBufferLayout,
    size: ^wgpu.Extent3D,
) {
    wgpu.QueueWriteTexture(
        gpu.queue,
        texture,
        data,
        len(data^),
        data_layout,
        size,
    )
}

