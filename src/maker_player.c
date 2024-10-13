#include <maker/maker_player.h>

#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <maker/maker_util.h>

#include <stdint.h>

#include "maker_internal.h"

// >> private
// PRIVATE: read packet
_MAKER_PRIVATE void _mk_player_open_stream(MKMedia *media) {
  AVFormatContext *ctx = avformat_alloc_context();
  if (avformat_open_input(&ctx, media->filename, NULL, NULL) != 0) {
  }

  for (unsigned int i = 0; i < ctx->nb_streams; i++) {
    AVStream *stream = ctx->streams[i];
    AVCodecParameters *params = stream->codecpar;

    if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
      media->video.has_stream = true;
      media->video.width = params->width;
      media->video.height = params->height;
    }
  }

  avformat_free_context(ctx);
}

_MAKER_PRIVATE int _mk_player_read_thread(void *data) {
  MKPlayer *player = data;

  for (;;) {
    mk_mutex_lock(player->stop_mutex);
    bool stop_requested = player->stop_requested;
    mk_mutex_unlock(player->stop_mutex);
    if (stop_requested) {
      break;
    }

    for (uint32_t i = 1; i < player->media_pool.pool.size; i++) {
      MKMediaPoolItem *item = &player->media_pool.items[i];
      MKMedia *media = &item->media;
      if (!media->video.has_stream) {
        _mk_player_open_stream(media);
      } else {
        printf("it works!\n");
        return 0;
      }
    }
  }

  return 0;
}

// >> PUBLIC
MKPlayer *mk_player_alloc(void) {
  MKPlayer *player = maker_malloc(sizeof(MKPlayer));
  player->paused = true;
  player->stop_requested = false;
  player->stop_mutex = mk_mutex_create();
  MAKER_ASSERT(player->stop_mutex);

  mk_media_pool_init(&player->media_pool, 1);

  player->continue_read_thread = mk_cond_create();
  MAKER_ASSERT(player->continue_read_thread);

  player->read_packet_thread =
      mk_thread_create(_mk_player_read_thread, "read_packet_thread", player);

  return player;
}
