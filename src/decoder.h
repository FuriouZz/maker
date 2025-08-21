#ifndef MK_DECODER_H
#define MK_DECODER_H

#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "mutex.h"
#include "packet_queue.h"
#include "thread.h"

typedef struct MKDecoder {
    AVCodecContext* codec_context;
    MKPacketQueue* packet_queue;

    MKPacketQueueItem packet;

    int is_finished; /* boolean */
    int has_packet_pending; /* boolean */

    MKCond* is_empty_signal;
    MKThread thread;
} MKDecoder;

extern int mk_decoder_init(
    MKDecoder* decoder, AVCodecContext* codec_context,
    MKPacketQueue* packet_queue, MKCond* is_empty_signal
);

extern void mk_decoder_destroy(MKDecoder* decoder);

extern int mk_decoder_start(MKDecoder* decoder);

extern void mk_decoder_abort(MKDecoder* decoder, MKFrameQueue* frame_queue);
#endif
