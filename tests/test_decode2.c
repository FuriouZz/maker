#include "maker/maker_media.h"
#include "maker/maker_thread.h"
#include <maker/maker_player.h>
#include <sokol_log.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  MKPlayer *player = mk_player_alloc();
  mk_media_pool_init(&player->media_pool, 1);
  mk_media_create(&player->media_pool, "./tests/video.mp4");
  int status;
  mk_thread_wait(player->read_packet_thread, &status);

  return status;
}
