package pipeline

import "vendor:wgpu"

HelloWorldPipeline :: struct {
    module:          wgpu.ShaderModule,
    pipeline_layout: wgpu.PipelineLayout,
    render_pipeline: wgpu.RenderPipeline,
}

create_hello_world :: proc(device: wgpu.Device) -> HelloWorldPipeline {
    code :: `
	@vertex
	fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
		let x = f32(i32(in_vertex_index) - 1);
		let y = f32(i32(in_vertex_index & 1u) * 2 - 1);
		return vec4<f32>(x, y, 0.0, 1.0);
	}

	@fragment
	fn fs_main() -> @location(0) vec4<f32> {
		return vec4<f32>(1.0, 0.0, 0.0, 1.0);
	}
	`

    shader_module := wgpu.DeviceCreateShaderModule(
        device,
        &{
            nextInChain = &wgpu.ShaderSourceWGSL {
                sType = .ShaderSourceWGSL,
                code = code,
            },
        },
    )

    render_pipeline_layout := wgpu.DeviceCreatePipelineLayout(device, &{})
    render_pipeline := wgpu.DeviceCreateRenderPipeline(
        device,
        &{
            layout = render_pipeline_layout,
            vertex = {module = shader_module, entryPoint = "vs_main"},
            fragment = &{
                module = shader_module,
                entryPoint = "fs_main",
                targetCount = 1,
                targets = &wgpu.ColorTargetState {
                    format = .BGRA8Unorm,
                    writeMask = wgpu.ColorWriteMaskFlags_All,
                },
            },
            primitive = {topology = .TriangleList},
            multisample = {count = 1, mask = 0xFFFFFFFF},
        },
    )

    return HelloWorldPipeline {
        module = shader_module,
        pipeline_layout = render_pipeline_layout,
        render_pipeline = render_pipeline,
    }
}

release_hello_world :: proc(pipeline: HelloWorldPipeline) {
    wgpu.RenderPipelineRelease(pipeline.render_pipeline)
    wgpu.PipelineLayoutRelease(pipeline.pipeline_layout)
    wgpu.ShaderModuleRelease(pipeline.module)
}

draw_hello_world :: proc(
    pipeline: HelloWorldPipeline,
    encoder: wgpu.RenderPassEncoder,
) {
    wgpu.RenderPassEncoderSetPipeline(encoder, pipeline.render_pipeline)
    wgpu.RenderPassEncoderDraw(
        encoder,
        vertexCount = 3,
        instanceCount = 1,
        firstVertex = 0,
        firstInstance = 0,
    )
}

