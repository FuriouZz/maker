#ifndef MAKER_FORMAT_H
#define MAKER_FORMAT_H

#include <libavutil/pixfmt.h>

typedef enum MKPixelFormat {
  MK_PXFMT_DEFAULT = -1,
  MK_PXFMT_RGBA,
  MK_PXFMT_RGB,
} MKPixelFormat;

extern enum AVPixelFormat mk_format_get_av_pixel_format(MKPixelFormat px_fmt);
#endif
