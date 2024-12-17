#include "maker/maker_media.h"
#include <maker/maker_player.h>
#include <sokol_log.h>

#define MAKER_PLAY_EXT_IMPL

#include "maker_play_ext.h"

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  MKMediaPool pool = {0};
  MKPlayer player = {0};

  mk_media_pool_init(&pool, 1);
  MKMediaHandle handle = mk_media_create(&pool, "./tests/video.mp4");
  MKMedia *media = mk_media_get(&pool, &handle);

  mk_player_open_media(&player, media);
  mk_player_decode_one(&player);

  save_pgm(&player, "./tmp/output.pgm");
  save_ppm(&player, "./tmp/output.ppm");

  return 0;
}
