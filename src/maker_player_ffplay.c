#include <maker/maker_player_ffplay.h>

#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/frame.h>
#include <maker/maker_mutex.h>
#include <maker/maker_util.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "maker/maker_thread.h"
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

_MAKER_PRIVATE int mk_player_packet_queue_init(MKPlayerPacketQueue *queue) {
  maker_clear(queue, sizeof(MKPlayerFrameQueue));

  queue->packet_list =
      av_fifo_alloc2(1, sizeof(MKPlayerPacketList), AV_FIFO_FLAG_AUTO_GROW);
  if (!queue->packet_list) {
    return -1;
  }

  queue->mutex = mk_mutex_create();
  if (!queue->mutex) {
    return -1;
  }

  queue->cond = mk_cond_create();
  if (!queue->cond) {
    return -1;
  }

  queue->abort_request = 1;

  return 0;
}

_MAKER_PRIVATE void mk_player_packet_queue_flush(MKPlayerPacketQueue *queue) {
  MKPlayerPacketList list;
  mk_mutex_lock(queue->mutex);
  while (av_fifo_read(queue->packet_list, &list, 1) >= 0) {
    av_packet_free(&list.pkt);
  }
  queue->nb_packet = 0;
  queue->size = 0;
  queue->duration = 0;
  queue->serial++;
  mk_mutex_unlock(queue->mutex);
}

_MAKER_PRIVATE void mk_player_packet_queue_destroy(MKPlayerPacketQueue *queue) {
  mk_player_packet_queue_flush(queue);
  av_fifo_freep2(&queue->packet_list);
  mk_mutex_destroy(queue->mutex);
  mk_cond_destroy(queue->cond);
}

_MAKER_PRIVATE void mk_player_packet_queue_abort(MKPlayerPacketQueue *queue) {
  mk_mutex_lock(queue->mutex);
  queue->abort_request = 1;
  mk_cond_signal(queue->cond);
  mk_mutex_unlock(queue->mutex);
}

_MAKER_PRIVATE void mk_player_packet_queue_start(MKPlayerPacketQueue *queue) {
  mk_mutex_lock(queue->mutex);
  queue->abort_request = 0;
  queue->serial++;
  mk_cond_signal(queue->cond);
  mk_mutex_unlock(queue->mutex);
}

_MAKER_PRIVATE int mk_player_packet_queue_put_private(
    MKPlayerPacketQueue *queue, AVPacket *packet
) {
  int ret;

  if (queue->abort_request) {
    return -1;
  }

  MKPlayerPacketList list;
  list.pkt = packet;
  list.serial = queue->serial;

  ret = av_fifo_write(queue->packet_list, &list, 1);
  if (ret < 0) {
    return ret;
  }

  queue->nb_packet++;
  queue->size += list.pkt->size + sizeof(list);
  queue->duration += list.pkt->duration;

  mk_cond_signal(queue->cond);

  return 0;
}

_MAKER_PRIVATE int
mk_player_packet_queue_put(MKPlayerPacketQueue *queue, AVPacket *packet) {
  int ret;
  AVPacket *tempPacket = av_packet_alloc();
  if (!tempPacket) {
    av_packet_unref(packet);
    return -1;
  }

  av_packet_move_ref(tempPacket, packet);

  mk_mutex_lock(queue->mutex);
  ret = mk_player_packet_queue_put_private(queue, tempPacket);
  mk_mutex_unlock(queue->mutex);

  if (ret < 0) {
    av_packet_free(&tempPacket);
  }

  return ret;
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
_MAKER_PRIVATE int mk_player_packet_queue_get(
    MKPlayerPacketQueue *queue, AVPacket *packet, int block, int *serial
) {
  int ret;
  MKPlayerPacketList list;

  mk_mutex_lock(queue->mutex);

  for (;;) {
    if (queue->abort_request) {
      ret = -1;
      break;
    }

    ret = av_fifo_read(queue->packet_list, &list, 1);

    if (ret >= 0) {
      queue->nb_packet--;
      queue->size -= list.pkt->size + sizeof(list);
      queue->duration -= list.pkt->duration;
      av_packet_move_ref(packet, list.pkt);
      if (serial) {
        *serial = list.serial;
      }
      ret = 1;
      break;
    } else if (!block) {
      ret = 0;
      break;
    } else {
      mk_cond_wait(queue->cond, queue->mutex);
    }
  }

  mk_mutex_unlock(queue->mutex);
  return ret;
}

_MAKER_PRIVATE void mk_player_frame_queue_signal(MKPlayerFrameQueue *queue) {
  mk_mutex_lock(queue->mutex);
  mk_cond_signal(queue->cond);
  mk_mutex_unlock(queue->mutex);
}

_MAKER_PRIVATE int mk_player_decoder_init(
    MKPlayerDecoder *decoder, AVCodecContext *codec, MKPlayerPacketQueue *queue,
    MKCond *empty_queue_cond
) {
  maker_clear(decoder, sizeof(*decoder));
  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    return -1;
  }
  decoder->packet = packet;
  decoder->queue = queue;
  decoder->codec = codec;
  decoder->start_pts = AV_NOPTS_VALUE;
  decoder->paquet_serial = -1;
  decoder->empty_queue_cond = empty_queue_cond;
  return 0;
}

_MAKER_PRIVATE void
decoder_abort(MKPlayerDecoder *decoder, MKPlayerFrameQueue *queue) {
  mk_player_packet_queue_abort(decoder->queue);
  mk_player_frame_queue_signal(queue);
  mk_thread_wait(decoder->decoder_thread, NULL);
  decoder->decoder_thread = NULL;
  mk_player_packet_queue_flush(decoder->queue);
}

_MAKER_PRIVATE void mk_player_decoder_free(MKPlayerDecoder *decoder) {
  if (decoder->packet) {
    av_packet_free(&decoder->packet);
  }

  if (decoder->codec) {
    avcodec_free_context(&decoder->codec);
  }
}

_MAKER_PRIVATE int mk_player_decoder_start(
    MKPlayerDecoder *decoder, int (*fn)(void *), char *thread_name, void *arg
) {
  mk_player_packet_queue_start(decoder->queue);
  decoder->decoder_thread = mk_thread_create(fn, thread_name, arg);
  if (!decoder->decoder_thread) {
    return -1;
  }
  return 0;
}

_MAKER_PRIVATE int mk_player_video_thread(void *data) {
  (void)(data);
  return 0;
}

_MAKER_PRIVATE int mk_player_stream_open(MKPlayerState *state, MKMedia *media) {
  AVFormatContext *format = avformat_alloc_context();
  if (!format) {
    MK_PLAYER_PANIC(AVFORMAT_ALLOC_CONTEXT_FAILED);
  }

  if (avformat_open_input(&format, media->filename, NULL, NULL) != 0) {
    MK_PLAYER_PANIC(AVFORMAT_OPEN_INPUT_FAILED);
  }

  if (avformat_find_stream_info(format, NULL) < 0) {
    MK_PLAYER_PANIC(AVFORMAT_FIND_STREAM_INFO_FAILED);
  }

  for (unsigned int i = 0; i < format->nb_streams; i++) {
    AVStream *stream = format->streams[i];
    AVCodecParameters *params = stream->codecpar;

    if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
      media->video.width = params->width;
      media->video.height = params->height;
      state->video.has_stream = true;
      state->video.stream_index = i;
      state->video.params = params;
    }
  }

  state->format = format;

  const AVCodec *video_codec =
      avcodec_find_decoder(state->video.params->codec_id);
  if (video_codec == NULL) {
    MK_PLAYER_PANIC(AVCODEC_FIND_CODEC_FAILED);
  }

  AVCodecContext *video_codec_context = avcodec_alloc_context3(video_codec);
  if (!video_codec_context) {
    return -1;
  }

  MKPlayerDecoder decoder;
  if (mk_player_decoder_init(
          &decoder, video_codec_context, &state->video.packet_queue,
          state->continue_read_thread
      ) < 0) {
    goto fail;
  }

  if (mk_player_decoder_start(
          &decoder, mk_player_video_thread, "video_thread", state
      ) < 0) {
    goto out;
  }

fail:
  avcodec_free_context(&video_codec_context);
out:

  return 0;
}

_MAKER_PRIVATE void mk_player_stream_close(MKPlayerState *state) {
  if (state->format) {
    avformat_free_context(state->format);
  }
}

_MAKER_PRIVATE int mk_player_read_thread(void *data) {
  MKPlayerState *state = data;

  for (uint32_t i = 1; i < state->media_pool.pool.size; i++) {
    MKMediaPoolItem *item = &state->media_pool.items[i];
    MKMedia *media = &item->media;
    if (!state->video.has_stream) {
      mk_player_stream_open(state, media);
    }
  }

  MKMediaPoolItem *item = &(state->media_pool.items[0]);
  if (!item || !state->video.has_stream) {
    goto fail;
  }

  int ret;
  AVPacket *packet = av_packet_alloc();
  MKMutex *wait_mutex = mk_mutex_create();

  if (!wait_mutex) {
    goto fail;
  }

  for (;;) {
    mk_mutex_lock(state->stop_mutex);
    bool stop_requested = state->stop_requested;
    mk_mutex_unlock(state->stop_mutex);

    if (stop_requested) {
      break;
    }

    ret = av_read_frame(state->format, packet);
    if (ret < 0) {

      if (state->video.stream_index == packet->stream_index) {
        mk_player_packet_queue_put(&state->video.packet_queue, packet);
      }

      mk_mutex_lock(wait_mutex);
      mk_cond_timedwait(state->continue_read_thread, wait_mutex, 10);
      mk_mutex_unlock(wait_mutex);
    } else {
      printf("eof end of file");
    }

    break;
  }

fail:

  if (state->video.has_stream) {
    mk_player_stream_close(state);
  }

  return 0;
}

// >> PUBLIC
void mk_player_setup(MKPlayerDesc *desc) { mk_player_global.desc = *desc; }

MKPlayerState *mk_player_alloc(void) {
  MKPlayerState *state = maker_malloc(sizeof(MKPlayerState));
  state->paused = true;
  state->stop_requested = false;
  state->stop_mutex = mk_mutex_create();
  MAKER_ASSERT(state->stop_mutex);

  mk_media_pool_init(&state->media_pool, 1);

  state->continue_read_thread = mk_cond_create();
  MAKER_ASSERT(state->continue_read_thread);

  state->read_packet_thread =
      mk_thread_create(mk_player_read_thread, "read_packet_thread", state);

  if (mk_player_frame_queue_init(
          &state->video.frame_queue, &state->video.packet_queue,
          VIDEO_PICTURE_QUEUE_SIZE, 1
      ) < 0) {
    goto fail;
  }

  if (mk_player_packet_queue_init(&state->video.packet_queue) < 0) {
    goto fail;
  }

fail:
  mk_player_free(state);
  state = NULL;

  return state;
}

void mk_player_free(MKPlayerState *state) {
  if (state->stop_mutex) {
    mk_mutex_destroy(state->stop_mutex);
  }
  if (state->media_pool.pool.valid) {
    mk_media_pool_free(&state->media_pool);
  }
  if (state->continue_read_thread) {
    mk_cond_destroy(state->continue_read_thread);
  }
  if (state->read_packet_thread) {
    mk_thread_wait(state->read_packet_thread, NULL);
  }
  maker_free(state);
}
