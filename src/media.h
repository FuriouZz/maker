#ifndef MK_MEDIA_H
#define MK_MEDIA_H

#include "libavformat/avformat.h"
#include "maker/maker.h"

typedef struct MKMediaContext {
    AVFormatContext* format;
} MKMediaContext;

#endif
