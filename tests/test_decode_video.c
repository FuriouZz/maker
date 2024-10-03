
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"

#define MAKER_DEBUG
#include "../src/maker_decoder.h"
#include "../src/shaders/quad.glsl.h"

static struct {
    sg_pass_action pass_action;
    sg_pipeline pipeline;
    sg_bindings bindings;
    const char *filename;
} state;

static void init(void) {
    mdecoder_setup(&(mdecoder_desc){.logger.func = slog_func});

    const mdecoder_media media = mdecoder_create_media(state.filename);
    mdecoder_decode_media(&media);
    const mdecoder_image_data data = mdecoder_alloc_image_data(&media);
    mdecoder_get_pixels(&data);

    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    state.bindings.fs.images[SLOT_tex] = sg_alloc_image();
    state.bindings.fs.samplers[SLOT_smp] = sg_make_sampler(&(sg_sampler_desc){
        .wrap_u = SG_WRAP_REPEAT,
        .wrap_v = SG_WRAP_REPEAT,
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .compare = SG_COMPAREFUNC_NEVER,
    });

    sg_init_image(
        state.bindings.fs.images[SLOT_tex],
        &(sg_image_desc){.width = data.width,
                         .height = data.height,
                         .pixel_format = SG_PIXELFORMAT_RGBA8,
                         .data.subimage[0][0] = {.ptr = data.buffer,
                                                 .size = data.buffer_size}});

    // a vertex buffer
    float vertices[] = {
        // clang-format off
        // positions            colors
        -0.5f,  0.5f, 0.5f,     1.0f, 0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, 0.5f,     0.0f, 1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, 0.5f,     1.0f, 1.0f, 0.0f, 1.0f,
        // clang-format on
    };
    state.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc){
        .data = SG_RANGE(vertices), .label = "quad-vertices"});

    // an index buffer with 2 triangles
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    state.bindings.index_buffer =
        sg_make_buffer(&(sg_buffer_desc){.type = SG_BUFFERTYPE_INDEXBUFFER,
                                         .data = SG_RANGE(indices),
                                         .label = "quad-indices"});

    sg_shader shd = sg_make_shader(quad_shader_desc(sg_query_backend()));

    state.pipeline = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .index_type = SG_INDEXTYPE_UINT16,
        .layout = {.attrs =
                       {
                           [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
                           [ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4,
                       }},
        .label = "quad-pipeline"});

    state.pass_action = (sg_pass_action){
        .colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                      .clear_value = {0.0f, 0.0f, 0.0f, 1.0f}}};
}

static void frame(void) {
    sg_begin_pass(&(sg_pass){.action = state.pass_action,
                             .swapchain = sglue_swapchain()});
    sg_apply_pipeline(state.pipeline);
    sg_apply_bindings(&state.bindings);
    sg_draw(0, 6, 1);
    sg_end_pass();
    sg_commit();
}

static void cleanup(void) { sg_shutdown(); }

sapp_desc sokol_main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    state.filename = argv[1];
    printf("%s", state.filename);
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .width = 800,
        .height = 600,
        .logger.func = slog_func,
    };
}
