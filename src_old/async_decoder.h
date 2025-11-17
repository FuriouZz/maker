#ifndef MK_DECODER_H
#define MK_DECODER_H

#include "maker_internal.h"

typedef struct MKVideoDecoder {
    AVPacket*       packet;
    AVCodecContext* codec_context;
    MKPacketQueue   packet_q;
    MKFrameQueue    frame_q;
    MKThread        thread;
    int32           is_finished;
} MKAsyncVideoDecoder;

typedef struct MKDemuxer {
    MKCond   continue_signal;
    MKThread thread;
} MKAsyncDemuxer;

typedef struct MKDecoderDesc {
    MKMedia* media;
    int*     is_aborted;
} MKAsyncDecoderDesc;

typedef struct MKAsyncDecoder {
    MKMedia*            media;
    MKAsyncVideoDecoder video;
    MKAsyncDemuxer      demuxer;

    int  is_eof;
    int* is_aborted;
} MKAsyncDecoder;

extern int32 mk_async_decoder_init(MKAsyncDecoder* decoder, MKAsyncDecoderDesc* desc);
extern int32 mk_async_decoder_destroy(MKAsyncDecoder* decoder);
extern int32 mk_async_decoder_start(MKAsyncDecoder* decoder);
extern int32 mk_async_decoder_stop(MKAsyncDecoder* decoder);

#endif
