#ifndef MAKER_PLAYER_H
#define MAKER_PLAYER_H

#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>
#include <libavformat/avformat.h>
#include <libavutil/fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <maker/maker_media.h>

typedef struct MKPlayerVideoStream {
  bool has_stream;
  int stream_index;
  AVCodecContext *codec;
  int width;
  int height;
} MKPlayerVideoStream;

typedef struct MKPlayerAudioStream {
  int sample_rate;
} MKPlayerAudioStream;

typedef struct MKPlayerDecoder {
  AVFrame *yuv_frame;
} MKPlayerDecoder;

typedef struct MKPlayer {
  AVFormatContext *format_context;
  MKPlayerVideoStream video;
  MKPlayerAudioStream audio;
  MKPlayerDecoder decoder;
} MKPlayer;

typedef struct MKPlayerImageData {
  uint8_t *buffer;
  int buffer_size;
  int width;
  int height;
  AVFrame *frame;
  enum AVPixelFormat format;
  struct SwsContext *sws_context;
} MKPlayerImageData;

extern void mk_player_open_media(MKPlayer *player, MKMedia *media);

extern int mk_player_decode(MKPlayer *player);

extern int mk_player_decode_one(MKPlayer *player);

extern void
mk_player_create_image_data(MKPlayer *player, MKPlayerImageData *image_data);

extern int mk_player_get_pixel(MKPlayer *player, MKPlayerImageData *image_data);
#endif
