#ifndef MK_CONTEXT_H
#define MK_CONTEXT_H

#include "async_decoder.h"
#include "clock.h"
#include "maker/maker.h"

typedef struct MKVideoOutput {
    MKImageData image_data;
    MKPixelFormat pixel_format;
    struct SwsContext* sws_context;
    AVFrame* frame;
} MKVideoOutput;

typedef struct MKInternalContext {
    MKAsyncDecoder decoder;
    MKClock clock;

    MKVideoOutput video_output;

    int is_aborted;
    int next_pts;
} MKInternalContext;

#endif
