#include <maker/maker_format.h>

enum AVPixelFormat mk_format_get_av_pixel_format(MKPixelFormat px_fmt) {
  switch (px_fmt) {
  case MK_PXFMT_RGB: {
    return AV_PIX_FMT_RGB24;
  }
  case MK_PXFMT_RGBA: {
    return AV_PIX_FMT_RGBA;
  }
  default:
  case MK_PXFMT_DEFAULT: {
    return AV_PIX_FMT_NONE;
  }
  }
}
