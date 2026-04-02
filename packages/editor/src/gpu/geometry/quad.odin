package geometry

import GPU "../"
import "vendor:wgpu"

create_quad :: proc(
    self: ^GPU.Context,
    width: f32 = 1,
    height: f32 = 1,
) -> (
    geometry: Geometry,
) {
    vertices: []Vertex_Position = {
        {position = {-width, -height, 0}},
        {position = {-width, height, 0}},
        {position = {width, height, 0}},
        {position = {width, -height, 0}},
    }

    indices := []u32{0, 1, 2, 0, 2, 3}

    geometry.vertex_buffer = wgpu.DeviceCreateBufferWithDataSlice(
        self.device,
        &{label = "Sprite Vertex Buffer", usage = {.Vertex}},
        vertices[:],
    )

    geometry.index_buffer = wgpu.DeviceCreateBufferWithDataSlice(
        self.device,
        &{label = "Sprite Index Buffer", usage = {.Index}},
        indices[:],
    )

    geometry.vertex_count = u32(len(vertices[:]))
    geometry.index_count = u32(len(indices[:]))

    geometry.vertex_size = size_of([4]Vertex_Position)
    geometry.index_size = size_of([6]u32)
    geometry.index_format = .Uint32

    return geometry
}

