#ifndef MK_FRAME_QUEUE_H
#define MK_FRAME_QUEUE_H

#include "libavutil/frame.h"
#include "packet_queue.h"

#define FRAME_QUEUE_SIZE 16

typedef struct MKFrameQueueItem {
    // Common
    AVFrame* frame;
    int serial; // for video/audio/subtitle sync
    int pts; // presentation timestamp for the frame
    int duration; // estimated duration of frame
    int64_t position; // byte position of the frame in the input file
    int format;

    // Video
    int width;
    int height;
    int flip_y; /* boolean */

    // Unsupported
    // AVSubtitle subtitle;
    // AVRational sar;
    // int uploaded;
} MKFrameQueueItem;

typedef struct MKFrameQueue {
    MKFrameQueueItem items[FRAME_QUEUE_SIZE];

    int read_index;
    int is_read_index_shown; /* boolean */
    int write_index;

    int frame_count;
    int max_frame_count;

    int keep_last_frame; /* boolean */

    MKPacketQueue* packet_queue;
    MKMutex mutex;
    MKCond update_signal;
} MKFrameQueue;

extern int mk_init_frame_queue(
    MKFrameQueue* queue, MKPacketQueue* packet_queue, int frame_count,
    int keep_last
);

extern void mk_uninit_frame_queue(MKFrameQueue* queue);

extern void mk_trigger_frame_queue_changes(MKFrameQueue* queue);

extern MKFrameQueueItem* mk_peek_frame(MKFrameQueue* queue);

extern MKFrameQueueItem* mk_peek_next_frame(MKFrameQueue* queue);

extern MKFrameQueueItem* mk_peek_last_frame(MKFrameQueue* queue);

extern MKFrameQueueItem* mk_peek_readable_frame(MKFrameQueue* queue);

extern MKFrameQueueItem* mk_peek_writable_frame(MKFrameQueue* queue);

extern void mk_push_writable_frame(MKFrameQueue* queue);

extern void mk_drop_frame(MKFrameQueue* queue);

extern int mk_remaining_frame_count(MKFrameQueue* queue);

// extern int64_t mk_frame_queue_get_last_shown_position(MKFrameQueue* queue);

#endif
