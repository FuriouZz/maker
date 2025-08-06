#ifndef MK_FFMPEG_DECODER_H
#define MK_FFMPEG_DECODER_H

#include "libavcodec/avcodec.h"
#include "maker/ffmpeg_frame_queue.h"
#include "maker/ffmpeg_packet_queue.h"
#include "maker/mutex.h"
#include "maker/thread.h"

typedef struct MKFFMpegDecoder {
    AVCodecContext* codec_context;
    MKFFMpegPacketQueue* packet_queue;

    MKFFMpegPacketQueueItem packet;

    int is_finished; /* boolean */
    int has_packet_pending; /* boolean */

    MKCond* is_empty_signal;
    MKThread* thread;

    int64_t start_pts;
} MKFFMpegDecoder;

extern int mk_ffmpeg_decoder_init(
    MKFFMpegDecoder* decoder, AVCodecContext* codec_context,
    MKFFMpegPacketQueue* packet_queue, MKCond* is_empty_signal
);

extern void mk_ffmpeg_decoder_destroy(MKFFMpegDecoder* decoder);

extern int mk_ffmpeg_decoder_start(
    MKFFMpegDecoder* decoder, MKThreadFunction callback,
    const char* thread_name, void* data
);

extern void mk_ffmpeg_decoder_abort(
    MKFFMpegDecoder* decoder, MKFFMpegFrameQueue* frame_queue
);
#endif
