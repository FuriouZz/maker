#include "stdio.h"
#include "microui.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"

sapp_desc sokol_main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  printf("MicroUI Version: %s\n", MU_VERSION);
  printf("AVFORMAT VERSION: %s\n", AV_STRINGIFY(LIBAVFORMAT_VERSION));
  printf("AVUTIL VERSION: %s\n", AV_STRINGIFY(LIBAVUTIL_VERSION));
  puts("Use sokol");
  return (sapp_desc){};
}
