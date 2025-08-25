#ifndef MK_DECODER_H
#define MK_DECODER_H

#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "maker/maker.h"
#include "mutex.h"
#include "packet_queue.h"
#include "thread.h"

typedef struct MKVideoDecoder {
    AVPacket* packet;
    AVCodecContext* codec_context;
    MKPacketQueue packet_q;
    MKFrameQueue frame_q;
    MKThread thread;
    int is_finished;
} MKAsyncVideoDecoder;

typedef struct MKDemuxer {
    MKCond continue_signal;
    MKThread thread;
} MKAsyncDemuxer;

typedef struct MKDecoderDesc {
    MKMedia* media;
    int* is_aborted;
} MKAsyncDecoderDesc;

typedef struct MKDecoder {
    MKMedia* media;
    MKAsyncVideoDecoder video;
    MKAsyncDemuxer demuxer;

    int is_eof;
    int* is_aborted;
} MKAsyncDecoder;

extern int
mk_async_decoder_init(MKAsyncDecoder* decoder, MKAsyncDecoderDesc* desc);

extern int mk_async_decoder_destroy(MKAsyncDecoder* decoder);

extern int mk_async_decoder_start(MKAsyncDecoder* decoder);

extern int mk_async_decoder_stop(MKAsyncDecoder* decoder);

#endif
