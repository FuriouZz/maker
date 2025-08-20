#ifndef MK_MEDIA_DECODER_H
#define MK_MEDIA_DECODER_H

#include "decoder.h"
#include "frame_queue.h"
#include "libswscale/swscale.h"
#include "maker/maker.h"
#include "packet_queue.h"
#include "thread.h"

typedef struct MKVideoDecoder {
    MKDecoder decoder;
    struct SwsContext* sws_context;
    int is_valid; /* boolean */
} MKVideoDecoder;

typedef struct MKAudioDecoder {
    int is_valid; /* boolean */
} MKAudioDecoder;

typedef struct MKDemuxer {
    MKCond continue_signal;
    MKThread thread;
    int is_eof; /* boolean */
} MKDemuxer;

typedef struct MKMediaDecoder {
    MKVideoDecoder video_decoder;
    MKAudioDecoder audio_decoder;
    MKMedia* media;
    int is_valid; /* boolean */

    MKPacketQueue packet_queue;
    MKFrameQueue frame_queue;
    MKCond is_empty_signal;
    int is_aborted;

    int video_stream_index;
    int audio_stream_index;
    MKDemuxer demuxer;
} MKMediaDecoder;

extern int mk_media_decoder_init(MKMediaDecoder* decoder, MKMedia* media);

extern void mk_media_decoder_free(MKMediaDecoder* decoder);

extern void mk_media_decoder_start(MKMediaDecoder* decoder);

extern void mk_media_decoder_stop(MKMediaDecoder* decoder);

extern int
mk_media_decoder_read_frame(MKMediaDecoder* decoder, MKImageData* target);

extern int
mk_media_decoder_read_frame2(MKMediaDecoder* decoder, MKImageData* target);

#endif
