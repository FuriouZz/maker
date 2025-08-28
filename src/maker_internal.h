#ifndef MK_CONTEXT2_H
#define MK_CONTEXT2_H

#include "decoder_pool.h"
#include "maker/maker.h"
#include "media_pool.h"
#include "message_queue.h"
#include "thread_manager.h"

typedef struct MKContext2 {
    MKMediaPool media_p;
    MKDecoderPool decoder_p;
    MKMessageQueue message_q;
    MKThreadManager thread_m;
} MKContext2;

#endif
