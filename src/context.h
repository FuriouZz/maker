#ifndef MK_CONTEXT_H
#define MK_CONTEXT_H

#include "clock.h"
#include "decoder2.h"
#include "maker/maker.h"

typedef struct MKVideoOutput {
    MKImageData image_data;
    MKPixelFormat pixel_format;
    struct SwsContext* sws_context;
    AVFrame* frame;
} MKVideoOutput;

typedef struct MKContext {
    MKDecoder2 decoder;
    MKClock clock;

    MKVideoOutput video_output;

    int is_aborted;
    int next_pts;
} MKContext;

#endif
