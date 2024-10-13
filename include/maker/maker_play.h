#ifndef MAKER_PLAY_H
#define MAKER_PLAY_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stdbool.h>

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
