package pipeline

import GPU "../"
import geom "../geometry"
import "core:fmt"
import "vendor:wgpu"

Sprite :: struct {
    // Buffers
    // projection_buffer:       wgpu.Buffer,

    // // Textures
    // depth_texture:           wgpu.Texture,
    // depth_texture_view:      wgpu.TextureView,

    // // Samplers
    // sampler:                 wgpu.Sampler,

    // // Bindings
    // constant_binding_layout: wgpu.BindGroupLayout,
    // constant_binding:        wgpu.BindGroup,

    // Bind Group Layouts
    bind_group_layout: wgpu.BindGroupLayout,

    // Pipeline
    pipeline:          wgpu.RenderPipeline,
    pipeline_layout:   wgpu.PipelineLayout,
    shader_module:     wgpu.ShaderModule,
}

@(private = "file")
SpriteVertex :: struct {
    x: f32,
    y: f32,
    z: f32,
}

@(private = "file")
SPRITE_SHADER_CODE :: `
struct VertexInput {
    @location(0) position: vec3<f32>,
}

struct VertexOutput {
    @location(0) tex_coords: vec2<f32>,
    @builtin(position) position: vec4<f32>,
}

@vertex
fn vs_main(model: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = vec4<f32>(model.position, 1.0);
    out.tex_coords = model.position.xy * 0.5 + 0.5;
    out.tex_coords.y = 1.0 - out.tex_coords.y;
    return out;
}

struct FragmentOutput {
    @location(0) color: vec4<f32>
}

@group(0) @binding(0)
var t_diffuse: texture_2d<f32>;

@group(0) @binding(1)
var s_diffuse: sampler;

@fragment
fn fs_main(in: VertexOutput) -> FragmentOutput {
    var out: FragmentOutput;
    out.color = textureSample(t_diffuse, s_diffuse, in.tex_coords);
    // out.color = vec4<f32>(1.0, 0.0, 0.0, 1.0);
    return out;
}
`

create_sprite_geometry :: proc() {
}

create_sprite :: proc(self: ^GPU.Context) -> (sprite: Sprite) {
    // Description
    vertex_attributes: []wgpu.VertexAttribute = {
        {shaderLocation = 0, format = .Float32x3, offset = 0},
    }

    vertex_buffer: []wgpu.VertexBufferLayout = {
        {
            stepMode = .Vertex,
            arrayStride = size_of(SpriteVertex),
            attributeCount = len(vertex_attributes),
            attributes = raw_data(vertex_attributes[:]),
        },
    }

    bind_group_layout_entries: []wgpu.BindGroupLayoutEntry = {
        {
            binding = 0,
            visibility = {.Fragment},
            texture = {
                multisampled = false,
                viewDimension = ._2D,
                sampleType = .Float,
            },
        },
        {binding = 1, visibility = {.Fragment}, sampler = {type = .Filtering}},
    }

    bind_group_layout_descs: []wgpu.BindGroupLayoutDescriptor = {
        {
            label = "Sprite Bind Group Layout #0",
            entryCount = 2,
            entries = raw_data(bind_group_layout_entries[:]),
        },
    }


    // bind_group_layouts: []wgpu.BindGroupLayout = {
    //     wgpu.DeviceCreateBindGroupLayout(
    //         self.device,
    //         &{entryCount = 2, entries = raw_data(bindings[:])},
    //     ),
    // }


    // {
    //     label = "Sprite Sampler",
    //     addressModeU = .ClampToEdge,
    //     addressModeV = .ClampToEdge,
    //     addressModeW = .ClampToEdge,
    //     magFilter = .Linear,
    //     minFilter = .Linear,
    //     maxAnisotropy = 1,
    // },

    // bind_group_entries: []wgpu.BindGroupEntry = {
    //     {binding = 0, textureView = nil},
    //     {binding = 1, sampler = sampler},
    // }

    // bind_groups: []wgpu.BindGroup = {
    //     wgpu.DeviceCreateBindGroup(
    //         self.device,
    //         &{
    //             label = "Sprite Bind Group",
    //             entries = raw_data(bindings[:]),
    //             entryCount = len(bindings[:]),
    //         },
    //     ),
    // }


    // instance_buffer := wgpu.DeviceCreateBuffer(
    //     self.device,
    //     &{usage = {.Vertex, .CopyDst}, size = 0},
    // )

    // sprite.projection_buffer = wgpu.DeviceCreateBuffer(
    //     self.device,
    //     &{usage = {.Uniform, .CopyDst}, size = size_of(matrix[4, 4]f32)},
    // )

    // sprite.sampler = wgpu.DeviceCreateSampler(
    //     self.device,
    //     &{
    //         addressModeU = .ClampToEdge,
    //         addressModeV = .ClampToEdge,
    //         addressModeW = .ClampToEdge,
    //         magFilter = .Linear,
    //         minFilter = .Linear,
    //         mipmapFilter = .Nearest,
    //         maxAnisotropy = 1,
    //     },
    // )

    // sprite.depth_texture = wgpu.DeviceCreateTexture(
    //     self.device,
    //     &{
    //         label = "init_sprite::create_texture",
    //         size = {width = 100, height = 100, depthOrArrayLayers = 1},
    //         mipLevelCount = 1,
    //         sampleCount = 1,
    //         dimension = ._2D,
    //         format = .Depth32Float,
    //         usage = {.TextureBinding},
    //     },
    // )

    // sprite.depth_texture_view = wgpu.TextureCreateView(sprite.depth_texture)

    // constant_layout_entries: []wgpu.BindGroupLayoutEntry = {
    //     // Texture Sampler
    //     {binding = 0, visibility = {.Fragment}, sampler = {type = .Filtering}},

    //     // Projection matrix
    //     {
    //         binding = 1,
    //         visibility = {.Vertex},
    //         buffer = {
    //             type = .Uniform,
    //             hasDynamicOffset = false,
    //             minBindingSize = size_of(matrix[4, 4]f32),
    //         },
    //     },
    // }

    // sprite.constant_binding_layout = wgpu.DeviceCreateBindGroupLayout(
    //     self.device,
    //     &{entries = raw_data(constant_layout_entries[:])},
    // )

    // constant_entries: []wgpu.BindGroupEntry = {
    //     // Texture Sampler
    //     {binding = 0, sampler = sprite.sampler},

    //     // Projection matrix
    //     {binding = 1, buffer = sprite.projection_buffer},
    // }

    // sprite.constant_binding = wgpu.DeviceCreateBindGroup(
    //     self.device,
    //     &{
    //         layout = sprite.constant_binding_layout,
    //         entries = raw_data(constant_entries[:]),
    //     },
    // )

    // instance_layout_entries: []wgpu.BindGroupLayoutEntry = {
    //     // Texture
    //     {
    //         binding = 0,
    //         visibility = .Fragment,
    //         texture = {
    //             sampleType = .Float,
    //             viewDimension = ._2D,
    //             multisampled = false,
    //         },
    //     },

    //     // Atlas size
    //     {
    //         binding = 1,
    //         visibility = {.Vertex, .Fragment},
    //         buffer = {
    //             type = .Uniform,
    //             hasDynamicOffset = false,
    //             minBindingSize = size_of([2]f32),
    //         },
    //     },
    // }

    // instance_binding_layout := wgpu.DeviceCreateBindGroupLayout(
    //     self.device,
    //     &{entries = raw_data(instance_layout_entries[:])},
    // )

    // instance_entries : []wgpu.BindGroupEntry = {
    //     { binding=0,textureView=depth_view },
    //     {binding=1,buffer=}
    // }

    // bind_group_layouts: []wgpu.BindGroupLayout = {
    //     sprite.constant_binding_layout,
    //     // instance_binding_layout,
    // }

    // Initialization
    sprite.bind_group_layout = wgpu.DeviceCreateBindGroupLayout(
        self.device,
        &bind_group_layout_descs[0],
    )


    bind_group_layouts: []wgpu.BindGroupLayout = {sprite.bind_group_layout}
    sprite.pipeline_layout = wgpu.DeviceCreatePipelineLayout(
        self.device,
        &{
            label = "Sprite Pipeline Layout",
            bindGroupLayoutCount = len(bind_group_layouts[:]),
            bindGroupLayouts = raw_data(bind_group_layouts[:]),
        },
    )

    sprite.shader_module = wgpu.DeviceCreateShaderModule(
        self.device,
        &{
            label = "Sprite Shader Module",
            nextInChain = &wgpu.ShaderSourceWGSL {
                sType = .ShaderSourceWGSL,
                code = SPRITE_SHADER_CODE,
            },
        },
    )

    render_targets: []wgpu.ColorTargetState = {
        {
            format = GPU.PREFERED_TEXTURE_FORMAT,
            writeMask = wgpu.ColorWriteMaskFlags_All,
        },
    }
    sprite.pipeline = wgpu.DeviceCreateRenderPipeline(
        self.device,
        &{
            label = "Sprite Pipeline",
            layout = sprite.pipeline_layout,
            vertex = {
                entryPoint = "vs_main",
                module = sprite.shader_module,
                bufferCount = len(vertex_buffer),
                buffers = raw_data(vertex_buffer[:]),
            },
            fragment = &{
                entryPoint = "fs_main",
                module = sprite.shader_module,
                targetCount = len(render_targets),
                targets = raw_data(render_targets[:]),
            },
            primitive = {topology = .TriangleList},
            multisample = {
                count = 1,
                mask = 0xFFFFFF,
                alphaToCoverageEnabled = false,
            },
        },
    )


    return sprite
}

// @(private = "file")
// get_sprite_info :: proc() -> (info: PipelineInfo) {

//     info.samplers = {
//         {
//             label = "Sprite Sampler",
//             addressModeU = .ClampToEdge,
//             addressModeV = .ClampToEdge,
//             addressModeW = .ClampToEdge,
//             magFilter = .Linear,
//             minFilter = .Linear,
//             maxAnisotropy = 1,
//         },
//     }

//     return info
// }

sprite_release :: proc(self: ^Sprite) {
    wgpu.RenderPipelineRelease(self.pipeline)
    wgpu.PipelineLayoutRelease(self.pipeline_layout)
    wgpu.ShaderModuleRelease(self.shader_module)
    // wgpu.BindGroupRelease(self.constant_binding)
    // wgpu.BindGroupLayoutRelease(self.constant_binding_layout)
    // wgpu.TextureViewRelease(self.depth_texture_view)
    // wgpu.TextureRelease(self.depth_texture)
    // wgpu.SamplerRelease(self.sampler)
    // wgpu.BufferRelease(self.projection_buffer)
}

sprite_draw :: proc(
    self: ^Sprite,
    pass: wgpu.RenderPassEncoder,
    geometry: geom.Geometry,
    bind_group: wgpu.BindGroup,
) {
    wgpu.RenderPassEncoderSetPipeline(pass, self.pipeline)
    wgpu.RenderPassEncoderSetVertexBuffer(
        pass,
        slot = 0,
        buffer = geometry.vertex_buffer,
        offset = 0,
        size = geometry.vertex_size,
    )

    wgpu.RenderPassEncoderSetBindGroup(
        pass,
        groupIndex = 0,
        group = bind_group,
        dynamicOffsets = nil,
    )

    if (geometry.index_buffer != nil) {
        wgpu.RenderPassEncoderSetIndexBuffer(
            pass,
            buffer = geometry.index_buffer,
            format = geometry.index_format,
            offset = 0,
            size = geometry.index_size,
        )

        wgpu.RenderPassEncoderDrawIndexed(
            pass,
            indexCount = geometry.index_count,
            instanceCount = 1,
            firstIndex = 0,
            baseVertex = 0,
            firstInstance = 0,
        )
    } else {
        wgpu.RenderPassEncoderDraw(
            pass,
            vertexCount = geometry.vertex_count,
            instanceCount = 1,
            firstVertex = 0,
            firstInstance = 0,
        )
    }
}

sprite_create_bind_group :: proc(
    self: ^Sprite,
    gpu: ^GPU.Context,
    texture_view: wgpu.TextureView,
    sampler: wgpu.Sampler,
) -> (
    wgpu.BindGroup,
    []wgpu.BindGroupEntry,
) {

    entries: []wgpu.BindGroupEntry = {
        {binding = 0, textureView = texture_view},
        {binding = 1, sampler = sampler},
    }

    bind_group := wgpu.DeviceCreateBindGroup(
        gpu.device,
        &{
            label = "Sprite Bind Group #0",
            layout = self.bind_group_layout,
            entryCount = len(entries[:]),
            entries = raw_data(entries[:]),
        },
    )

    return bind_group, entries
}

