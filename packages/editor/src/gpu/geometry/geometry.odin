package geometry

import "base:intrinsics"
import "vendor:wgpu"

Geometry :: struct {
    vertex_buffer: wgpu.Buffer,
    vertex_count:  u32,
    vertex_size:   u64,
    index_buffer:  wgpu.Buffer, /*  NULLABLE */
    index_count:   u32,
    index_size:    u64,
    index_format:  wgpu.IndexFormat,
}

Vertex_Position :: struct {
    position: [3]f32,
}

Vertex_Position_Texcoords :: struct {
    position:  [3]f32,
    texcoords: [3]f32,
}

release :: proc(self: ^$T) where intrinsics.type_is_subtype_of(T, Geometry) {
    wgpu.BufferRelease(self.vertex_buffer)
    if (self.index_buffer != nil) {
        wgpu.BufferRelease(self.index_buffer)
    }
}

