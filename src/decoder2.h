#ifndef MK_DECODER2_H
#define MK_DECODER2_H

#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "maker/maker.h"
#include "mutex.h"
#include "packet_queue.h"
#include "thread.h"

typedef struct MKVideoDecoder2 {
    AVPacket* packet;
    AVCodecContext* codec_context;
    MKPacketQueue packet_q;
    MKFrameQueue frame_q;
    MKThread thread;
    int is_finished;
} MKVideoDecoder2;

typedef struct MKDemuxer2 {
    MKCond continue_signal;
    MKThread thread;
} MKDemuxer2;

typedef struct MKDecoder2Desc {
    MKMedia* media;
    int* is_eof;
    int* is_aborted;
} MKDecoder2Desc;

typedef struct MKDecoder2 {
    MKMedia* media;
    MKVideoDecoder2 video;
    MKDemuxer2 demuxer;

    int* is_eof;
    int* is_aborted;
} MKDecoder2;

extern int mk_decoder2_init(MKDecoder2* decoder, MKDecoder2Desc* desc);

extern int mk_decoder2_destroy(MKDecoder2* decoder);

extern int mk_decoder2_start(MKDecoder2* decoder);

extern int mk_decoder2_stop(MKDecoder2* decoder);

#endif
