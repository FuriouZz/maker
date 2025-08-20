#ifndef MK_PACKET_QUEUE_H
#define MK_PACKET_QUEUE_H

#include "libavcodec/packet.h"
#include "libavutil/fifo.h"
#include "mutex.h"

typedef struct MKPacketQueueItem {
    AVPacket* packet;
    int serial; // for video/audio/subtitle sync
} MKPacketQueueItem;

typedef struct MKPacketQueue {
    AVFifo* items;
    int packet_count;
    int duration;
    int byte_size;
    int is_aborted; /* boolean */
    int serial; // for video/audio/subtitle sync
    MKMutex mutex;
    MKCond new_item_signal;
} MKPacketQueue;

extern int mk_packet_queue_init(MKPacketQueue* queue);

extern void mk_packet_queue_destroy(MKPacketQueue* queue);

extern void mk_packet_queue_start(MKPacketQueue* queue);

extern void mk_packet_queue_abort(MKPacketQueue* queue);

extern void mk_packet_queue_flush(MKPacketQueue* queue);

extern int mk_packet_queue_put(MKPacketQueue* queue, AVPacket* packet);

extern int mk_packet_queue_get(
    MKPacketQueue* queue, AVPacket* packet, int should_block, int* serial
);

#endif
