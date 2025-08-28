#ifndef MK_MESSAGE_QUEUE_H
#define MK_MESSAGE_QUEUE_H

#include "maker/maker.h"
#include "mutex.h"

typedef enum MKMessageKind {
    MK_MESSAGEKIND_UNKNOWN,
    MK_MESSAGEKIND_DEMUX,
    MK_MESSAGEKIND_SEEK,
    MK_MESSAGEKIND_DECODE_VIDEO,
} MKMessageKind;

typedef struct MKMessageQueue {
    void*   fifo;
    MKMutex mutex;
} MKMessageQueue;

typedef struct MKMessageData {
    MKMessageKind kind;
    union {
        MKDecoderHandle handle;
        struct {
            MKDecoderHandle handle;
            double          time;
        } seek;
    } data;
} MKMessageData;

extern void mk_message_queue_init(MKMessageQueue* queue);
extern void mk_message_queue_uninit(MKMessageQueue* queue);
extern int  mk_message_queue_send_message(MKMessageQueue* queue, MKMessageData* data);
extern int  mk_message_queue_get_message(MKMessageQueue* queue, MKMessageData* data);
extern int  mk_message_queue_has_messages(MKMessageQueue* queue);

#endif
