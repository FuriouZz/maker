#ifndef MK_DECODER_H
#define MK_DECODER_H

#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#include "maker/image_data.h"
#include "maker/media.h"

typedef struct MKVideoDecoder {
    AVCodecContext* codec_context;
    AVFrame* frame;
    struct SwsContext* sws_context;
    int is_valid; /* boolean */
} MKVideoDecoder;

typedef struct MKAudioDecoder {
    AVCodecContext* codec_context;
    int is_valid; /* boolean */
} MKAudioDecoder;

typedef struct MKDecoder {
    MKVideoDecoder video_decoder;
    MKAudioDecoder audio_decoder;
    MKMedia* media;
    int is_valid; /* boolean */
} MKDecoder;

extern int mk_decoder_start(MKDecoder* decoder, MKMedia* media);

extern void mk_decoder_stop(MKDecoder* decoder);

extern int mk_decoder_read_frame(MKDecoder* decoder, MKImageData* target);

#endif
