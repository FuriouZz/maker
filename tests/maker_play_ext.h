#ifndef MAKER_PLAY_EXT_H
#define MAKER_PLAY_EXT_H

#include "maker/maker_player.h"
#include <maker/maker_play.h>

extern void save_pgm(MKPlayer *player, char *output);

extern void save_ppm(MKPlayer *player, char *output);

#endif

#ifdef MAKER_PLAY_EXT_IMPL

#include <libavutil/imgutils.h>

static void _save_gray_frame(
    unsigned char *buf, int wrap, int xsize, int ysize, char *filename
) {
  FILE *f;
  int i;
  f = fopen(filename, "wb");
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
  for (i = 0; i < ysize; i++) {
    fwrite(buf + i * wrap, 1, xsize, f);
  }
  fclose(f);
}

static void _save_rgb_frame(
    unsigned char *buf, int wrap, int xsize, int ysize, char *filename
) {
  FILE *f;
  int i;
  f = fopen(filename, "wb");
  fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);
  for (i = 0; i < ysize; i++) {
    fwrite(buf + i * wrap, 1, xsize * 3, f);
  }
  fclose(f);
}

void save_pgm(MKPlayer *player, char *output) {
  AVFrame *src_frame = player->decoder.yuv_frame;
  _save_gray_frame(
      src_frame->data[0], src_frame->linesize[0], player->video.width,
      player->video.height, output
  );
}

void save_ppm(MKPlayer *player, char *output) {
  AVFrame *src_frame = player->decoder.yuv_frame;
  const int format = AV_PIX_FMT_RGB24;
  AVCodecContext *codec_context = player->video.codec;
  struct SwsContext *sws_context = sws_getContext(
      codec_context->width, codec_context->height, codec_context->pix_fmt,
      codec_context->width, codec_context->height, format, SWS_BILINEAR, NULL,
      NULL, NULL
  );
  AVFrame *dst_frame = av_frame_alloc();

  av_image_alloc(
      dst_frame->data, dst_frame->linesize, player->video.width,
      player->video.height, format, 1
  );

  sws_scale(
      sws_context, (const uint8_t *const *)src_frame->data, src_frame->linesize,
      0, player->video.height, dst_frame->data, dst_frame->linesize
  );

  _save_rgb_frame(
      dst_frame->data[0], dst_frame->linesize[0], player->video.width,
      player->video.height, output
  );

  av_frame_free(&dst_frame);
  sws_freeContext(sws_context);
}
#endif
