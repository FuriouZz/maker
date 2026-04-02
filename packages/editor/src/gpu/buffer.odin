package gpu

import "base:intrinsics"
import "vendor:wgpu"

write_buffer_slice :: proc(self: ^Context, dst: wgpu.Buffer, data: []$T) {
    source := create_buffer_with_data_slice(self, .CopySrc, data)
    wgpu.CommandEncoderCopyBufferToBuffer(
        self.encoder,
        &source,
        0,
        dst,
        0,
        len(data),
    )
}

write_buffer_typed :: proc(
    self: ^Context,
    dst: wgpu.Buffer,
    data: $T,
) where !intrinsics.type_is_sliceable($T) {
    source := create_buffer_with_data_typed(self, .CopySrc, data)
    wgpu.CommandEncoderCopyBufferToBuffer(
        self.encoder,
        source,
        0,
        dst,
        0,
        len(data),
    )
}

write_buffer :: proc {
    write_buffer_slice,
    write_buffer_typed,
}

enqueue_write_buffer :: proc(
    self: ^Context,
    buffer: wgpu.Buffer,
    offset: u64,
    data: ^[]byte,
) {
    wgpu.QueueWriteBuffer(self.queue, buffer, offset, data, len(data^))
}

