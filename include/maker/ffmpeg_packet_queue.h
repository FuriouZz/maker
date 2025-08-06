#ifndef MK_FFMPEG_PACKET_QUEUE_H
#define MK_FFMPEG_PACKET_QUEUE_H

#include "libavcodec/packet.h"
#include "libavutil/fifo.h"
#include "maker/mutex.h"

typedef struct MKFFMpegPacketQueueItem {
    AVPacket* packet;
    int serial; // for video/audio/subtitle sync
} MKFFMpegPacketQueueItem;

typedef struct MKFFMpegPacketQueue {
    AVFifo* items;
    int packet_count;
    int duration;
    int byte_size;
    int is_aborted; /* boolean */
    int serial; // for video/audio/subtitle sync
    MKMutex* mutex;
    MKCond* new_item_signal;
} MKFFMpegPacketQueue;

extern int mk_ffmpeg_packet_queue_init(MKFFMpegPacketQueue* queue);

extern void mk_ffmpeg_packet_queue_free(MKFFMpegPacketQueue* queue);

extern void mk_ffmpeg_packet_queue_start(MKFFMpegPacketQueue* queue);

extern void mk_ffmpeg_packet_queue_abort(MKFFMpegPacketQueue* queue);

extern void mk_ffmpeg_packet_queue_flush(MKFFMpegPacketQueue* queue);

extern int
mk_ffmpeg_packet_queue_put(MKFFMpegPacketQueue* queue, AVPacket* packet);

extern int mk_ffmpeg_packet_queue_get(
    MKFFMpegPacketQueue* queue, AVPacket* packet, int should_block, int* serial
);

#endif
