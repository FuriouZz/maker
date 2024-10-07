
#include "stdint.h"
#include "stdio.h"

#include "libavutil/imgutils.h"

#include "maker_internal.h"
#include "maker_play.h"
#include "maker_util.h"

// >>structs
typedef struct {
  bool setup_called;
  mk_play_desc desc;
} _mk_play_state;

_MAKER_PRIVATE _mk_play_state _mk_play;

// >>logging
#if defined(MAKER_DEBUG)
#define _MK_PLAY_LOGITEM_XMACRO(item, msg) #item ": " msg,
_MAKER_PRIVATE const char *_mk_play_log_messages[] = {_MK_PLAY_LOG_ITEMS};
#undef _MK_PLAY_LOGITEM_XMACRO
#endif

// clang-format off
#define _MK_PLAY_PANIC(code) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 0, 0, __LINE__)
#define _MK_PLAY_ERROR(code) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 1, 0, __LINE__)
#define _MK_PLAY_ERRORMSG(code,msg) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 1, msg, __LINE__)
#define _MK_PLAY_WARN(code) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 2, 0, __LINE__)
#define _MK_PLAY_WARNMSG(code,msg) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 2, msg, __LINE__)
#define _MK_PLAY_INFO(code) _mk_play_log(MK_PLAY_LOGITEM_ ##code, 3, 0, __LINE__)
// clang-format on

_MAKER_PRIVATE void _mk_play_log(
    mk_play_log_item log_item, uint32_t log_level, const char *msg,
    uint32_t line_nr
) {
  if (_mk_play.desc.logger.func) {
    const char *filename = 0;
#if defined(MAKER_DEBUG)
    filename = __FILE__;
    if (0 == msg) {
      msg = _mk_play_log_messages[log_item];
    }
#endif
    _mk_play.desc.logger.func(
        "maker_play", log_level, log_item, msg, line_nr, filename,
        _mk_play.desc.logger.user_data
    );
  } else {
    // for log level PANIC it would be 'undefined behaviour' to continue
    if (log_level == 0) {
      abort();
    }
  }
}

// >>debugging
#if defined(MAKER_DEBUG)
#include "maker_play_debug.h"
#endif

// >>resources
_MAKER_PRIVATE AVFormatContext *_mdecoder_alloc_format_context(void) {
  AVFormatContext *format_context = avformat_alloc_context();
  if (!format_context) {
    _MK_PLAY_PANIC(AVFORMAT_ALLOC_FAILED);
  }
  return format_context;
}

_MAKER_PRIVATE AVFormatContext *_mdecoder_open_file(const char *filename) {
  AVFormatContext *format_context = _mdecoder_alloc_format_context();

  if (avformat_open_input(&format_context, filename, NULL, NULL) != 0) {
    _MK_PLAY_PANIC(AVFORMAT_OPEN_FILE_FAILED);
  }

  if (avformat_find_stream_info(format_context, NULL) < 0) {
    _MK_PLAY_PANIC(AVFORMAT_FIND_STREAM_INFO_FAILED);
  }

  return format_context;
}

_MAKER_PRIVATE AVCodecContext *
_mdecoder_alloc_codec_context(const AVCodec *codec) {
  AVCodecContext *codec_context = avcodec_alloc_context3(codec);
  if (!codec_context) {
    _MK_PLAY_PANIC(AVCODEC_ALLOC_CONTEXT_FAILED);
  }
  return codec_context;
}

_MAKER_PRIVATE AVFrame *_mdecoder_alloc_frame(void) {
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    _MK_PLAY_PANIC(AVUTIL_FRAME_ALLOC_FAILED);
  }
  return frame;
}

_MAKER_PRIVATE AVPacket *_mdecoder_alloc_packet(void) {
  AVPacket *packet = av_packet_alloc();
  if (!packet) {
    _MK_PLAY_PANIC(AVUTIL_PACKET_ALLOC_FAILED);
  }
  return packet;
}

_MAKER_PRIVATE int _mk_play_decode_packet(
    AVPacket *packet, AVFrame *frame, AVCodecContext *context
) {
  char buffer[1024];

  int response = 0;
  response = avcodec_send_packet(context, packet);

  if (response < 0) {
    snprintf(
        buffer, sizeof(buffer),
        "Error while sending a packet to the decoder: %s\n",
        av_err2str(response)
    );
    _MK_PLAY_WARNMSG(AVCODEC_SEND_PACKET_FAILED, buffer);
    return response;
  }

  while (response >= 0) {
    response = avcodec_receive_frame(context, frame);

    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    }

    if (response < 0) {
      snprintf(
          buffer, sizeof(buffer),
          "Error while receiving a frame from the decoder: %s\n",
          av_err2str(response)
      );
      _MK_PLAY_WARNMSG(AVCODEC_RECEIVE_FRAME_FAILED, buffer);
      return response;
    }

    if (response == 0)
      break;
  }

  return 0;
}

_MAKER_PRIVATE enum AVPixelFormat
_mk_play_get_av_pixel_format(mk_play_pixel_format px_fmt) {
  switch (px_fmt) {
  case MK_PLAY_PXFMT_RGB: {
    return AV_PIX_FMT_RGB24;
  }
  case MK_PLAY_PXFMT_RGBA: {
    return AV_PIX_FMT_RGBA;
  }
  default:
  case MK_PLAY_PXFMT_UNKNOWN: {
    return AV_PIX_FMT_NONE;
  }
  }
}

// >> public
void mk_play_setup(const mk_play_desc *desc) {
  MAKER_ASSERT(!_mk_play.setup_called);
  MAKER_ASSERT(desc);
  _mk_play.desc = *desc;
}

mk_play_decode_context mk_play_alloc_decode_context(
    const mk_play_media *media, mk_play_pixel_format px_fmt
) {
  MAKER_ASSERT(media->video.has_stream);
  enum AVPixelFormat format = _mk_play_get_av_pixel_format(px_fmt);
  MAKER_ASSERT(format > -1);

  int err;

  AVCodecContext *codec_context =
      _mdecoder_alloc_codec_context(media->video.codec);

  AVStream *stream = media->format_context->streams[media->video.stream_index];
  AVCodecParameters *codec_parameters = stream->codecpar;

  err = avcodec_parameters_to_context(codec_context, codec_parameters);
  if (err < 0) {
    _MK_PLAY_ERROR(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED);
  }

  err = avcodec_open2(codec_context, media->video.codec, NULL);
  if (err < 0) {
    _MK_PLAY_ERRORMSG(AVCODEC_OPEN_CODEC_FAILED, av_err2str(err));
    _MK_PLAY_PANIC(AVCODEC_OPEN_CODEC_FAILED);
  }

  struct SwsContext *sws_context = sws_getContext(
      codec_context->width, codec_context->height, codec_context->pix_fmt,
      codec_context->width, codec_context->height, format, SWS_BILINEAR, NULL,
      NULL, NULL
  );

  mk_play_decode_context ctx = {
      .codec_context = codec_context,
      .sws_context = sws_context,
      .frame = av_frame_alloc(),
      .pixel_format = format,
  };
  return ctx;
}

void mk_play_free_decode_context(const mk_play_decode_context *context) {
  AVFrame *frame = context->frame;
  AVCodecContext *codec_context = context->codec_context;
  avcodec_free_context(&codec_context);
  av_frame_free(&frame);
}

mk_play_media mk_play_alloc_media(const char *filename) {
  MAKER_ASSERT(filename);

  mk_play_media media = {.filename = filename};

  AVFormatContext *format_context = _mdecoder_open_file(media.filename);
  media.format_context = format_context;

  // printf(
  //     "format %s, duration %lld us, bit_rate %lld\n",
  //     format_context->iformat->name, format_context->duration,
  //     format_context->bit_rate
  // );

  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    AVStream *stream = format_context->streams[i];

#if defined(MAKER_DEBUG)
    _mk_play_debug_stream(stream);
#endif

    AVCodecParameters *codec_parameters = stream->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
    if (codec == NULL) {
      _MK_PLAY_PANIC(AVCODEC_FIND_CODEC_FAILED);
    }

#if defined(MAKER_DEBUG)
    _mk_play_debug_codec(codec, stream);
#endif

    if (codec->type == AVMEDIA_TYPE_VIDEO && !media.video.has_stream) {
      media.video.codec = codec;
      media.video.stream_index = i;
      media.video.width = codec_parameters->width;
      media.video.height = codec_parameters->height;
      media.video.has_stream = true;
    }

    if (codec->type == AVMEDIA_TYPE_AUDIO && !media.audio.has_stream) {
      media.audio.codec = codec;
      media.audio.stream_index = i;
      media.audio.has_stream = true;
    }
  }

  return media;
}

void mk_play_free_media(const mk_play_media *media) {
  avformat_free_context(media->format_context);
}

mk_play_image_data
mk_play_alloc_image_data(int width, int height, mk_play_pixel_format px_fmt) {
  enum AVPixelFormat format = _mk_play_get_av_pixel_format(px_fmt);
  MAKER_ASSERT(format > -1);

  int size = av_image_get_buffer_size(format, width, height, 1);
  mk_play_image_data data = {
      .buffer = malloc(size * sizeof(uint8_t)),
      .buffer_size = size,
      .width = width,
      .height = height,
      .format = format,
  };
  return data;
}

void mk_play_free_image_data(const mk_play_image_data *image_data) {
  free(image_data->buffer);
}

_MAKER_PRIVATE int64_t FrameToPts(AVStream *pavStream, int frame) {
  int64_t target_dts_usecs = (int64_t)round(
      frame * (double)pavStream->r_frame_rate.den /
      pavStream->r_frame_rate.num * AV_TIME_BASE
  );
  int64_t first_dts_usecs = (int64_t)round(
      pavStream->pts_wrap_bits * (double)pavStream->time_base.num /
      pavStream->time_base.den * AV_TIME_BASE
  );
  target_dts_usecs += first_dts_usecs;

  return target_dts_usecs;
}

int mk_play_seek(
    const mk_play_decode_context *decode_context, const mk_play_media *media,
    int64_t index
) {
  AVStream *stream = media->format_context->streams[media->video.stream_index];

  double stream_timebase = av_q2d(stream->time_base) * 1000.0 * 10000.0;
  long start_time = stream->start_time != AV_NOPTS_VALUE
                        ? (long)(stream->start_time * stream_timebase)
                        : 0;
  double avg_frame_duration = 10000000 / av_q2d(stream->avg_frame_rate);

  long frame_timestamp = (long)(index * avg_frame_duration);

  int result = av_seek_frame(
      media->format_context, -1, (frame_timestamp + start_time) / 10,
      AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD
  );

  char buffer[1024];
  AVPacket *packet = av_packet_alloc();
  AVFrame *frame = decode_context->frame;

  for (;;) {
    result = av_read_frame(media->format_context, packet);

    // Hangle EOF/error
    if (result != 0) {
      break;
    }

    // Accept only video stream
    if (packet->stream_index != media->video.stream_index) {
      av_packet_unref(packet);
      continue;
    }

    // Send packet for decoding
    result = avcodec_send_packet(decode_context->codec_context, packet);

    // Handle EOF/error
    if (result != 0) {
      snprintf(
          buffer, sizeof(buffer),
          "Error while sending a packet to the decoder: %s\n",
          av_err2str(result)
      );
      _MK_PLAY_WARNMSG(AVCODEC_SEND_PACKET_FAILED, buffer);
      av_packet_unref(packet);
      break;
    }

    for (;;) {
      result = avcodec_receive_frame(decode_context->codec_context, frame);

      if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
        av_frame_unref(frame);
        break;
      }

      if (result < 0) {
        snprintf(
            buffer, sizeof(buffer),
            "Error while receiving a frame from the decoder: %s\n",
            av_err2str(result)
        );
        _MK_PLAY_WARNMSG(AVCODEC_RECEIVE_FRAME_FAILED, buffer);
        av_frame_unref(frame);
        break;
      }

      // Get frame pts (prefer best_effort_timestamp)
      long current_pts = frame->best_effort_timestamp == AV_NOPTS_VALUE
                             ? frame->pts
                             : frame->best_effort_timestamp;
      if (current_pts == AV_NOPTS_VALUE) {
        av_frame_unref(frame);
        continue;
      }

      printf(
          "[Skip] [pts %ld] [time: %ld]\n", current_pts,
          current_pts * (long)stream_timebase
      );

      if ((long)(current_pts * stream_timebase) / 10000 <
          frame_timestamp / 10000) {
        av_frame_unref(frame);
        continue;
      }

      return 0;
    }
  }

  return result;
}

void mk_play_decode(
    const mk_play_decode_context *context, const mk_play_media *media
) {
  AVFrame *frame = context->frame;
  AVPacket *packet = av_packet_alloc();
  AVCodecContext *codec_context = context->codec_context;

  int response = 0;
  int packet_count = 1;
  int i = 0;

  avcodec_flush_buffers(context->codec_context);

  while (av_read_frame(media->format_context, packet) >= 0) {
    if (packet->stream_index == (int)media->video.stream_index) {
      response = _mk_play_decode_packet(packet, frame, codec_context);
      if (response < 0) {
        break;
      }

      if (++i > packet_count) {
        break;
      }
    }
    av_packet_unref(packet);
  }

  av_packet_free(&packet);
}

void mk_play_get_pixels(
    const mk_play_decode_context *context, const mk_play_image_data *data
) {
  AVFrame *src_frame = context->frame;
  struct SwsContext *sws_context = context->sws_context;
  AVFrame *dst_frame = av_frame_alloc();

  if (context->codec_context->width != data->width ||
      context->codec_context->height != data->height) {
    _MK_PLAY_ERRORMSG(
        AV_IMAGE_COPY_TO_BUFFER_FAILED,
        "ImageData does not match AVCodecContext's dimensions."
    );
    _MK_PLAY_PANIC(AV_IMAGE_COPY_TO_BUFFER_FAILED);
  }
  if (context->pixel_format != data->format) {
    _MK_PLAY_ERRORMSG(
        AV_IMAGE_COPY_TO_BUFFER_FAILED,
        "ImageData does not match SwsContext's pixel format."
    );
    _MK_PLAY_PANIC(AV_IMAGE_COPY_TO_BUFFER_FAILED);
  }

  av_image_alloc(
      dst_frame->data, dst_frame->linesize, context->codec_context->width,
      context->codec_context->height, data->format, 1
  );

  sws_scale(
      sws_context, (const uint8_t *const *)src_frame->data, src_frame->linesize,
      0, context->codec_context->height, dst_frame->data, dst_frame->linesize
  );

  int ret = av_image_copy_to_buffer(
      data->buffer, data->buffer_size, (const uint8_t *const *)dst_frame->data,
      dst_frame->linesize, data->format, data->width, data->height, 1
  );

  if (ret < 0) {
    char str[512];
    maker_strfmt(str, MAKER_LEN(str), "%s", av_err2str(ret));
    _MK_PLAY_ERRORMSG(AV_IMAGE_COPY_TO_BUFFER_FAILED, str);
    _MK_PLAY_PANIC(AV_IMAGE_COPY_TO_BUFFER_FAILED);
  }

  av_frame_free(&dst_frame);
}
