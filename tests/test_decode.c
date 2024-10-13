#include <maker/maker_play.h>
#include <sokol_log.h>

#define MAKER_PLAY_EXT_IMPL

#include "maker_play_ext.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  mk_play_setup(&(mk_play_desc){.logger.func = slog_func});

  const mk_play_media media = mk_play_alloc_media("./tests/video.mp4");
  const mk_play_decode_context context =
      mk_play_alloc_decode_context(&media, MK_PLAY_PXFMT_RGB);
  mk_play_decode(&context, &media);
  mk_play_save_pgm(&context, "./tmp/output.pgm");
  mk_play_save_ppm(&context, "./tmp/output.ppm");

  return 0;
}
