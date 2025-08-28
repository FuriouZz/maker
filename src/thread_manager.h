#ifndef MK_THREAD_MANAGER_H
#define MK_THREAD_MANAGER_H

#include "decoder_pool.h"
#include "message_queue.h"
#include "mutex.h"
#include "thread.h"

typedef struct MKThreadContext {
    MKDecoderPool*  decoder_p;
    MKMessageQueue* msg_q;
    int*            is_aborted;
} MKThreadContext;

typedef struct MKThreadManager {
    MKThread        demuxer_thread;
    MKCond          demuxer_continue_signal;

    MKThread        video_thread;
    MKCond          video_continue_signal;

    MKThreadContext context;

    int             is_aborted;
} MKThreadManager;

extern void mk_thread_manager_init(MKThreadManager* manager, MKDecoderPool* pool, MKMessageQueue* queue);
extern void mk_thread_manager_uninit(MKThreadManager* manager);
extern int  mk_thread_manager_start(MKThreadManager* manager);
extern void mk_thread_manager_stop(MKThreadManager* manager);

#endif
