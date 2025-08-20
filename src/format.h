#ifndef MK_FORMAT_H
#define MK_FORMAT_H

#include "libavutil/pixfmt.h"
#include "maker/maker.h"

extern enum AVPixelFormat mk_format_to_av_pixel_format(MKPixelFormat px_fmt);

extern MKPixelFormat mk_format_from_av_pixel_format(enum AVPixelFormat px_fmt);

#endif
