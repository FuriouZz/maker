#include "maker_internal.h"

enum AVPixelFormat maker_format_to_av_pixel_format(MakerPixelFormat px_fmt)
{
    switch (px_fmt) {
    case MAKER_PIXEL_FORMAT_RGB: {
        return AV_PIX_FMT_RGB24;
    }
    case MAKER_PIXEL_FORMAT_RGBA: {
        return AV_PIX_FMT_RGBA;
    }
    case MAKER_PIXEL_FORMAT_YUV420P: {
        return AV_PIX_FMT_YUV420P;
    }
    default: {
        return AV_PIX_FMT_NONE;
    }
    }
}

MakerPixelFormat maker_format_from_av_pixel_format(enum AVPixelFormat px_fmt)
{
    switch (px_fmt) {
    case AV_PIX_FMT_RGB24: {
        return MAKER_PIXEL_FORMAT_RGB;
    }
    case AV_PIX_FMT_RGBA: {
        return MAKER_PIXEL_FORMAT_RGBA;
    }
    case AV_PIX_FMT_YUV420P: {
        return MAKER_PIXEL_FORMAT_YUV420P;
    }
    default: {
        return MAKER_PIXEL_FORMAT_UNKNOWN;
    }
    }
}
