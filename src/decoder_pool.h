#ifndef DECODER_POOL_H
#define DECODER_POOL_H

#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "maker/maker.h"
#include "media_pool.h"
#include "packet_queue.h"
#include "pool.h"

typedef struct MKVideoDecoder2 {
    AVFormatContext* format;
    AVCodecContext*  codec;
    AVPacket*        packet;

    MKPacketQueue    packet_q;
    MKFrameQueue     frame_q;

    int              is_finished;
} MKVideoDecoder2;

typedef struct MKDecoder {
    MKPoolSlot      slot;
    MKVideoDecoder2 video;
} MKDecoder;

typedef struct MKDecoderPool {
    MKDecoder* items;
    MKPool     pool;
} MKDecoderPool;

extern void            mk_decoder_pool_init(MKDecoderPool* pool);
extern void            mk_decoder_pool_uninit(MKDecoderPool* pool);
extern MKDecoderHandle mk_decoder_pool_alloc_decoder(MKDecoderPool* pool);
extern void            mk_decoder_pool_dealloc_decoder(MKDecoderPool* pool, MKDecoderHandle* handle);
extern void            mk_decoder_pool_init_decoder(MKDecoderPool* pool, MKDecoderHandle* handle, MKMedia2* media);
extern void            mk_decoder_pool_uninit_decoder(MKDecoderPool* pool, MKDecoderHandle* handle);

#endif
