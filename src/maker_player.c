#include <maker/maker_player.h>

#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/frame.h>
#include <maker/maker_mutex.h>
#include <maker/maker_util.h>

#include "maker_internal.h"

#include <stdint.h>

// >> private

typedef struct MKPlayerGlobal {
  MKPlayerDesc desc;
} MKPlayerGlobal;

MKPlayerGlobal mk_player_global;

// clang-format off
#define MK_PLAYER_LOGITEMS \
  MK_PLAYER_LOGITEM(OK, "Ok") \
  MK_PLAYER_LOGITEM(AVFORMAT_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVFormatContext") \
  MK_PLAYER_LOGITEM(AVFORMAT_OPEN_INPUT_FAILED, "Could not open file") \
  MK_PLAYER_LOGITEM(AVFORMAT_FIND_STREAM_INFO_FAILED, "Could not open file") \
  MK_PLAYER_LOGITEM(AVCODEC_FIND_CODEC_FAILED, "Could not find codec") \
  MK_PLAYER_LOGITEM(AVCODEC_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVCodecContext") \
  MK_PLAYER_LOGITEM(AVCODEC_OPEN_CODEC_FAILED, "Could not open codec") \
  MK_PLAYER_LOGITEM(AVUTIL_FRAME_ALLOC_FAILED, "Could not allocate memory for AVFrame") \
  MK_PLAYER_LOGITEM(AVUTIL_PACKET_ALLOC_FAILED, "Could not allocate memory for AVPacket") \
  MK_PLAYER_LOGITEM(AVCODEC_SEND_PACKET_FAILED, "Error while sending a packet to the decoder") \
  MK_PLAYER_LOGITEM(AVCODEC_RECEIVE_FRAME_FAILED, "Error while receiving a frame from the decoder") \
  MK_PLAYER_LOGITEM(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED, "Failed to copy codec params to codec context") \
  MK_PLAYER_LOGITEM(AV_IMAGE_COPY_TO_BUFFER_FAILED, "Failed to copy AVFrame to buffer") \


#define MK_PLAYER_LOGITEM(item, msg) MK_PLAYER_LOGITEM_##item,
typedef enum mk_player_log_item { MK_PLAYER_LOGITEMS } mk_player_log_item;
#undef MK_PLAYER_LOGITEM
// clang-format on

#if defined(MAKER_DEBUG)
#define MK_PLAYER_LOGITEM(item, msg) #item ": " msg,
_MAKER_PRIVATE const char *mk_player_log_messages[] = {MK_PLAYER_LOGITEMS};
#undef MK_PLAYER_LOGITEM
#endif

// clang-format off
#define MK_PLAYER_PANIC(code) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 0, 0, __LINE__)
#define MK_PLAYER_ERROR(code) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 1, 0, __LINE__)
#define MK_PLAYER_ERRORMSG(code,msg) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 1, msg, __LINE__)
#define MK_PLAYER_WARN(code) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 2, 0, __LINE__)
#define MK_PLAYER_WARNMSG(code,msg) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 2, msg, __LINE__)
#define MK_PLAYER_INFO(code) mk_player_log(MK_PLAYER_LOGITEM_ ##code, 3, 0, __LINE__)
// clang-format on

_MAKER_PRIVATE void mk_player_log(
    mk_player_log_item log_item, uint32_t log_level, const char *msg,
    uint32_t line_nr
) {
  if (mk_player_global.desc.logger.func) {
    const char *filename = 0;
#if defined(MAKER_DEBUG)
    filename = __FILE__;
    if (0 == msg) {
      msg = mk_player_log_messages[log_item];
    }
#endif
    mk_player_global.desc.logger.func(
        "maker_play", log_level, log_item, msg, line_nr, filename,
        mk_player_global.desc.logger.user_data
    );
  } else {
    // for log level PANIC it would be 'undefined behaviour' to continue
    if (log_level == 0) {
      abort();
    }
  }
}

_MAKER_PRIVATE int mk_player_frame_queue_init(
    MKPlayerFrameQueue *frameQueue, MKPlayerPacketQueue *packetQueue,
    int max_size, int keep_last
) {
  maker_clear(frameQueue, sizeof(MKPlayerFrameQueue));

  frameQueue->mutex = mk_mutex_create();
  if (!frameQueue->mutex) {
    return -1;
  }
  frameQueue->cond = mk_cond_create();
  if (!frameQueue->cond) {
    return -1;
  }
  frameQueue->pktq = packetQueue;
  frameQueue->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
  frameQueue->keep_last = !!keep_last;

  for (int i = 0; i < frameQueue->max_size; i++) {
    if (!(frameQueue->queue[i].frame = av_frame_alloc())) {
      return -1;
    }
  }

  return 0;
}
_MAKER_PRIVATE int mk_player_packet_queue_init(MKPlayerPacketQueue *packetQueue
) {
  maker_clear(packetQueue, sizeof(MKPlayerFrameQueue));

  packetQueue->packet_list =
      av_fifo_alloc2(1, sizeof(MKPlayerPacketList), AV_FIFO_FLAG_AUTO_GROW);
  if (!packetQueue->packet_list) {
    return -1;
  }

  packetQueue->mutex = mk_mutex_create();
  if (!packetQueue->mutex) {
    return -1;
  }

  packetQueue->cond = mk_cond_create();
  if (!packetQueue->cond) {
    return -1;
  }

  packetQueue->abort_request = 1;

  return 0;
}

_MAKER_PRIVATE void mk_player_open_streams(MKMedia *media) {
  AVFormatContext *ctx = avformat_alloc_context();
  if (!ctx) {
    MK_PLAYER_PANIC(AVFORMAT_ALLOC_CONTEXT_FAILED);
  }

  if (avformat_open_input(&ctx, media->filename, NULL, NULL) != 0) {
    MK_PLAYER_PANIC(AVFORMAT_OPEN_INPUT_FAILED);
  }

  if (avformat_find_stream_info(ctx, NULL) < 0) {
    MK_PLAYER_PANIC(AVFORMAT_FIND_STREAM_INFO_FAILED);
  }

  for (unsigned int i = 0; i < ctx->nb_streams; i++) {
    AVStream *stream = ctx->streams[i];
    AVCodecParameters *params = stream->codecpar;

    if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
      media->video.width = params->width;
      media->video.height = params->height;
      media->video.has_stream = true;
    }
  }

  media->format_context = ctx;
  media->is_opened = true;
}

_MAKER_PRIVATE void mk_player_free_media(MKMedia *media) {
  if (media->format_context) {
    avformat_free_context(media->format_context);
  }
}

_MAKER_PRIVATE int mk_player_read_thread(void *data) {
  MKPlayer *player = data;

  for (uint32_t i = 1; i < player->media_pool.pool.size; i++) {
    MKMediaPoolItem *item = &player->media_pool.items[i];
    MKMedia *media = &item->media;
    if (!media->is_opened) {
      mk_player_open_streams(media);
    }
  }

  if (mk_player_frame_queue_init(
          &player->video.frame_queue, &player->video.packet_queue,
          VIDEO_PICTURE_QUEUE_SIZE, 1
      ) < 0) {
    goto fail;
  }

  if (mk_player_packet_queue_init(&player->video.packet_queue) < 0) {
    goto fail;
  }

  for (;;) {
    mk_mutex_lock(player->stop_mutex);
    bool stop_requested = player->stop_requested;
    mk_mutex_unlock(player->stop_mutex);
    if (stop_requested) {
      break;
    }

    break;
  }

fail:

  for (uint32_t i = 1; i < player->media_pool.pool.size; i++) {
    MKMediaPoolItem *item = &player->media_pool.items[i];
    MKMedia *media = &item->media;
    if (media->is_opened) {
      mk_player_free_media(media);
    }
  }

  return 0;
}

// >> PUBLIC
void mk_player_setup(MKPlayerDesc *desc) { mk_player_global.desc = *desc; }

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
      mk_thread_create(mk_player_read_thread, "read_packet_thread", player);

  return player;
}

void mk_player_free(MKPlayer *player) {
  if (player->stop_mutex) {
    mk_mutex_destroy(player->stop_mutex);
  }
  if (player->media_pool.pool.valid) {
    mk_media_pool_free(&player->media_pool);
  }
  if (player->continue_read_thread) {
    mk_cond_destroy(player->continue_read_thread);
  }
  if (player->read_packet_thread) {
    mk_thread_wait(player->read_packet_thread, NULL);
  }
  maker_free(player);
}
