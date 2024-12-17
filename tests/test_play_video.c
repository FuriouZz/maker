#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_log.h>

#include <maker/maker_media.h>
#include <maker/maker_player.h>
#include <maker/shaders/quad.glsl.h>
#include <stdio.h>

static struct {
  sg_pass_action pass_action;
  sg_pipeline pipeline;
  sg_bindings bindings;
  const char *filename;

  MKPlayer player;
  MKMediaPool media_pool;
  MKPlayerImageData image_data;

  uint64_t time;
} state;

static void init(void) {
  MKMediaPool *pool = &state.media_pool;
  MKPlayer *player = &state.player;
  MKPlayerImageData *data = &state.image_data;

  mk_media_pool_init(pool, 1);
  mk_media_create(pool, state.filename);
  MKMedia *media = &pool->items[1].media;

  mk_player_open_media(player, media);
  mk_player_create_image_data(player, data);

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
      &(sg_image_desc){
          .width = data->width,
          .height = data->height,
          .pixel_format = SG_PIXELFORMAT_RGBA8,
          .usage = SG_USAGE_STREAM,
      }
  );

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
  state.bindings.vertex_buffers[0] = sg_make_buffer(&(sg_buffer_desc
  ){.data = SG_RANGE(vertices), .label = "quad-vertices"});

  // an index buffer with 2 triangles
  uint16_t indices[] = {0, 1, 2, 0, 2, 3};
  state.bindings.index_buffer = sg_make_buffer(&(sg_buffer_desc
  ){.type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices),
    .label = "quad-indices"});

  sg_shader shd = sg_make_shader(quad_shader_desc(sg_query_backend()));

  state.pipeline = sg_make_pipeline(&(sg_pipeline_desc
  ){.shader = shd,
    .index_type = SG_INDEXTYPE_UINT16,
    .layout =
        {.attrs =
             {
                 [ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
                 [ATTR_vs_color0].format = SG_VERTEXFORMAT_FLOAT4,
             }},
    .label = "quad-pipeline"});

  state.pass_action = (sg_pass_action
  ){.colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = {0.0f, 0.0f, 0.0f, 1.0f}
    }};
}

static void frame(void) {
  state.time = sapp_frame_count();

  if (state.time > 10 * 1000 * 1000) {
    state.time = 0;
  }

  mk_player_decode_one(&state.player);
  mk_player_get_pixel(&state.player, &state.image_data);
  sg_update_image(
      state.bindings.fs.images[SLOT_tex],
      &(sg_image_data
      ){.subimage[0][0] =
            {.ptr = state.image_data.buffer,
             .size = state.image_data.buffer_size}}
  );

  sg_begin_pass(&(sg_pass
  ){.action = state.pass_action, .swapchain = sglue_swapchain()});
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
  state.filename = "./tests/video.mp4";
  return (sapp_desc){
      .init_cb = init,
      .frame_cb = frame,
      .cleanup_cb = cleanup,
      .width = 800,
      .height = 600,
      .logger.func = slog_func,
  };
}
