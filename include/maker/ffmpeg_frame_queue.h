#ifndef MK_FFMPEG_FRAME_QUEUE_H
#define MK_FFMPEG_FRAME_QUEUE_H

#include "libavutil/frame.h"
#include "maker/ffmpeg_packet_queue.h"

#define FRAME_QUEUE_SIZE 16

typedef struct MKFFMpegFrameQueueItem {
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
} MKFFMpegFrameQueueItem;

typedef struct MKFFMpegFrameQueue {
    MKFFMpegFrameQueueItem items[FRAME_QUEUE_SIZE];

    int read_index;
    int is_read_index_shown; /* boolean */
    int write_index;

    int frame_count;
    int max_frame_count;

    int keep_last_frame; /* boolean */

    MKFFMpegPacketQueue* packet_queue;
    MKMutex* mutex;
    MKCond* update_signal;
} MKFFMpegFrameQueue;

extern int mk_ffmpeg_frame_queue_init(
    MKFFMpegFrameQueue* queue, MKFFMpegPacketQueue* packet_queue,
    int frame_count, int keep_last
);

extern void mk_ffmpeg_frame_queue_destroy(MKFFMpegFrameQueue* queue);

extern void mk_ffmpeg_frame_queue_unref_item(MKFFMpegFrameQueueItem* item);

extern void mk_ffmpeg_frame_queue_trigger_changes(MKFFMpegFrameQueue* queue);

extern MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek(MKFFMpegFrameQueue* queue);

extern MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_next(MKFFMpegFrameQueue* queue);

extern MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_last(MKFFMpegFrameQueue* queue);

extern MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_readable(MKFFMpegFrameQueue* queue);

extern MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_writable(MKFFMpegFrameQueue* queue);

extern void mk_ffmpeg_frame_queue_push(MKFFMpegFrameQueue* queue);

extern void mk_ffmpeg_frame_queue_next(MKFFMpegFrameQueue* queue);

extern int
mk_ffmpeg_frame_queue_remaining_frame_count(MKFFMpegFrameQueue* queue);

extern int64_t
mk_ffmpeg_frame_queue_get_last_shown_position(MKFFMpegFrameQueue* queue);

#endif
