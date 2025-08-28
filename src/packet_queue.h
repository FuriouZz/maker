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

extern int mk_init_packet_queue(MKPacketQueue* queue);

extern void mk_uninit_packet_queue(MKPacketQueue* queue);

extern void mk_start_packet_queue(MKPacketQueue* queue);

extern void mk_abort_packet_queue(MKPacketQueue* queue);

extern void mk_flush_packet_queue(MKPacketQueue* queue);

extern int mk_put_packet(MKPacketQueue* queue, AVPacket* packet);

extern int mk_get_packet_queue(
    MKPacketQueue* queue, AVPacket* packet, int should_block, int* serial
);

#endif
