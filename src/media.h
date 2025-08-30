#ifndef MK_MEDIA_H
#define MK_MEDIA_H

#include "libavformat/avformat.h"

typedef struct MKInternalMediaContext {
    AVFormatContext* format;
} MKInternalMediaContext;

#endif
