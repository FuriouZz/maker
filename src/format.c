#include "libavutil/pixfmt.h"
#include "maker/format.h"

enum AVPixelFormat mk_format_to_av_pixel_format(MKPixelFormat px_fmt)
{
    switch (px_fmt) {
    case MK_PXFMT_RGB: {
        return AV_PIX_FMT_RGB24;
    }
    case MK_PXFMT_RGBA: {
        return AV_PIX_FMT_RGBA;
    }
    default: {
        return AV_PIX_FMT_NONE;
    }
    }
}

MKPixelFormat mk_format_from_av_pixel_format(enum AVPixelFormat px_fmt)
{
    switch (px_fmt) {
    case AV_PIX_FMT_RGB24: {
        return MK_PXFMT_RGB;
    }
    case AV_PIX_FMT_RGBA: {
        return MK_PXFMT_RGBA;
    }
    default: {
        return MK_PXFMT_UNKNOWN;
    }
    }
}
