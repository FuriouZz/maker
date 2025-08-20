#ifndef MK_MEDIA_DECODER2_H
#define MK_MEDIA_DECODER2_H

#include "clock.h"
#include "decoder.h"
#include "frame_queue.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"
#include "maker/maker.h"
#include "packet_queue.h"

typedef struct MKMediaDecoderDesc {
    MKMedia* media;
    int use_yuv;
    int frame_rate;
} MKMediaAsyncDecoderDesc;

typedef struct MKMediaConverter {
    enum AVPixelFormat target_format;
    struct SwsContext* sws_context;
    AVFrame* frame;
} MKMediaConverter;

typedef struct MKMediaAsyncDecoder {
    MKMedia* media;
    int frame_rate;

    MKPacketQueue packet_queue;
    int is_aborted;
    int is_eof;
    double max_frame_duration; // maximum duration of a frame - above this, we
                               // consider the jump a timestamp discontinuity

    MKClock clock;

    MKThread demuxer_thread;
    MKCond continue_demux_signal;

    MKDecoder video_decoder;

    MKFrameQueue picture_queue;
    MKMediaConverter converter;
} MKMediaAsyncDecoder;

extern int mk_media_async_decoder_init(
    MKMediaAsyncDecoder* decoder, MKMediaAsyncDecoderDesc* desc
);
extern void mk_media_async_decoder_destroy(MKMediaAsyncDecoder* decoder);
extern int mk_media_async_decoder_start(MKMediaAsyncDecoder* decoder);
extern void mk_media_async_decoder_stop(MKMediaAsyncDecoder* decoder);
extern int mk_media_async_decoder_get_picture(
    MKMediaAsyncDecoder* decoder, MKImageData* target
);
extern int mk_media_async_refresh(MKMediaAsyncDecoder* decoder);

#endif
