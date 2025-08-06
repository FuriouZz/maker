#ifndef MK_FORMAT_H
#define MK_FORMAT_H

#include "libavutil/pixfmt.h"

typedef enum MKPixelFormat {
    MK_PXFMT_UNKNOWN = -1,
    MK_PXFMT_RGBA,
    MK_PXFMT_RGB,
} MKPixelFormat;

extern enum AVPixelFormat mk_format_to_av_pixel_format(MKPixelFormat px_fmt);

extern MKPixelFormat mk_format_from_av_pixel_format(enum AVPixelFormat px_fmt);

#endif
