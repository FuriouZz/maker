#ifndef MAKER_PLAY_EXT_H
#define MAKER_PLAY_EXT_H

#include "../src/maker_play.h"

extern void
mk_play_save_pgm(const mk_play_decode_context *context, char *output);

extern void
mk_play_save_ppm(const mk_play_decode_context *context, char *output);

#endif

#ifdef MAKER_PLAY_EXT_IMPL

#include "libavutil/imgutils.h"

static void _mk_play_save_gray_frame(
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

static void _mk_play_save_rgb_frame(
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

void mk_play_save_pgm(const mk_play_decode_context *context, char *output) {
  AVFrame *src_frame = context->frame;
  _mk_play_save_gray_frame(
      src_frame->data[0], src_frame->linesize[0], context->codec_context->width,
      context->codec_context->height, output
  );
}

void mk_play_save_ppm(const mk_play_decode_context *context, char *output) {
  AVFrame *src_frame = context->frame;
  struct SwsContext *sws_context = context->sws_context;
  AVFrame *dst_frame = av_frame_alloc();

  av_image_alloc(
      dst_frame->data, dst_frame->linesize, context->codec_context->width,
      context->codec_context->height, context->pixel_format, 1
  );

  sws_scale(
      sws_context, (const uint8_t *const *)src_frame->data, src_frame->linesize,
      0, context->codec_context->height, dst_frame->data, dst_frame->linesize
  );

  _mk_play_save_rgb_frame(
      dst_frame->data[0], dst_frame->linesize[0], context->codec_context->width,
      context->codec_context->height, output
  );

  av_frame_free(&dst_frame);
}
#endif
