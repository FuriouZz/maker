#ifndef MAKER_PLAY_H
#define MAKER_PLAY_H

#include "stdbool.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

// clang-format off
#define _MK_PLAY_LOG_ITEMS \
    _MK_PLAY_LOGITEM_XMACRO(OK, "Ok") \
    _MK_PLAY_LOGITEM_XMACRO(AVFORMAT_ALLOC_FAILED, "Could not allocate memory for AVFormatContext") \
    _MK_PLAY_LOGITEM_XMACRO(AVFORMAT_OPEN_FILE_FAILED, "Could not open file") \
    _MK_PLAY_LOGITEM_XMACRO(AVFORMAT_FIND_STREAM_INFO_FAILED, "Could not open file") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_FIND_CODEC_FAILED, "Could not find codec") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVCodecContext") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_OPEN_CODEC_FAILED, "Could not open codec") \
    _MK_PLAY_LOGITEM_XMACRO(AVUTIL_FRAME_ALLOC_FAILED, "Could not allocate memory for AVFrame") \
    _MK_PLAY_LOGITEM_XMACRO(AVUTIL_PACKET_ALLOC_FAILED, "Could not allocate memory for AVPacket") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_SEND_PACKET_FAILED, "Error while sending a packet to the decoder") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_RECEIVE_FRAME_FAILED, "Error while receiving a frame from the decoder") \
    _MK_PLAY_LOGITEM_XMACRO(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED, "Failed to copy codec params to codec context") \
    _MK_PLAY_LOGITEM_XMACRO(AV_IMAGE_COPY_TO_BUFFER_FAILED, "Failed to copy AVFrame to buffer") \

#define _MK_PLAY_LOGITEM_XMACRO(item, msg) MK_PLAY_LOGITEM_##item,
typedef enum mk_play_log_item { _MK_PLAY_LOG_ITEMS } mk_play_log_item;
#undef _MK_PLAY_LOGITEM_XMACRO
// clang-format on

typedef struct mk_play_logger {
  void (*func)(
      const char *tag,              // always "mk_play"
      uint32_t log_level,           // 0=panic, 1=error, 2=warning, 3=info
      uint32_t log_item_id,         // mk_play_LOGITEM_*
      const char *message_or_null,  // a message string, may be
                                    // nullptr in release mode
      uint32_t line_nr,             // line number in video_player.h
      const char *filename_or_null, // source filename, may be
                                    // nullptr in release mode
      void *user_data
  );
  void *user_data;
} mk_play_logger;

typedef struct mk_play_desc {
  mk_play_logger logger;
} mk_play_desc;

typedef struct mk_play_media {
  const char *filename;
  AVFormatContext *format_context;
  struct {
    bool has_stream;
    int stream_index;
    int width;
    int height;
    const AVCodec *codec;
  } video;
  struct {
    bool has_stream;
    int stream_index;
    const AVCodec *codec;
  } audio;
} mk_play_media;

typedef struct mk_play_image_data {
  uint8_t *buffer;
  int buffer_size;
  int width;
  int height;
  enum AVPixelFormat format;
} mk_play_image_data;

typedef struct mk_play_decode_context {
  AVFrame *frame;
  AVCodecContext *codec_context;
  struct SwsContext *sws_context;
  enum AVPixelFormat pixel_format;
} mk_play_decode_context;

typedef enum mk_play_pixel_format {
  MK_PLAY_PXFMT_UNKNOWN = -1,
  MK_PLAY_PXFMT_RGBA,
  MK_PLAY_PXFMT_RGB,
} mk_play_pixel_format;

extern void mk_play_setup(const mk_play_desc *desc);

extern mk_play_decode_context mk_play_alloc_decode_context(
    const mk_play_media *media, mk_play_pixel_format px_fmt
);

extern void mk_play_free_decode_context(const mk_play_decode_context *context);

extern mk_play_media mk_play_alloc_media(const char *filename);

extern void mk_play_free_media(const mk_play_media *media);

extern mk_play_image_data
mk_play_alloc_image_data(int width, int height, mk_play_pixel_format px_fmt);

extern void mk_play_free_image_data(const mk_play_image_data *image_data);

extern int mk_play_seek(
    const mk_play_decode_context *context, const mk_play_media *media,
    int64_t timestamp
);

extern void mk_play_decode(
    const mk_play_decode_context *decode_context, const mk_play_media *media
);

extern void mk_play_get_pixels(
    const mk_play_decode_context *context, const mk_play_image_data *data
);
#endif
