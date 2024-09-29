#ifndef MAKER_H
#define MAKER_H

#include "stdbool.h"

#include "libavcodec/codec.h"
#include "libavformat/avformat.h"

// clang-format off
#define _MPLAYER_LOG_ITEMS \
    _MPLAYER_LOGITEM_XMACRO(OK, "Ok") \
    _MPLAYER_LOGITEM_XMACRO(AVFORMAT_ALLOC_FAILED, "Could not allocate memory for AVFormatContext") \
    _MPLAYER_LOGITEM_XMACRO(AVFORMAT_OPEN_FILE_FAILED, "Could not open file") \
    _MPLAYER_LOGITEM_XMACRO(AVFORMAT_FIND_STREAM_INFO_FAILED, "Could not open file") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_FIND_CODEC_FAILED, "Could not find codec") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVCodecContext") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_OPEN_CODEC_FAILED, "Could not open codec") \
    _MPLAYER_LOGITEM_XMACRO(AVUTIL_FRAME_ALLOC_FAILED, "Could not allocate memory for AVFrame") \
    _MPLAYER_LOGITEM_XMACRO(AVUTIL_PACKET_ALLOC_FAILED, "Could not allocate memory for AVPacket") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_SEND_PACKET_FAILED, "Error while sending a packet to the decoder") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_RECEIVE_FRAME_FAILED, "Error while receiving a frame from the decoder") \
    _MPLAYER_LOGITEM_XMACRO(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED, "Failed to copy codec params to codec context") \
    _MPLAYER_LOGITEM_XMACRO(AV_IMAGE_COPY_TO_BUFFER_FAILED, "Failed to copy AVFrame to buffer") \

#define _MPLAYER_LOGITEM_XMACRO(item, msg) MPLAYER_LOGITEM_##item,
typedef enum mplayer_log_item { _MPLAYER_LOG_ITEMS } mplayer_log_item;
#undef _MPLAYER_LOGITEM_XMACRO
// clang-format on

typedef struct mplayer_logger {
    void (*func)(const char *tag,      // always "mplayer"
                 uint32_t log_level,   // 0=panic, 1=error, 2=warning, 3=info
                 uint32_t log_item_id, // MPLAYER_LOGITEM_*
                 const char *message_or_null,  // a message string, may be
                                               // nullptr in release mode
                 uint32_t line_nr,             // line number in video_player.h
                 const char *filename_or_null, // source filename, may be
                                               // nullptr in release mode
                 void *user_data);
    void *user_data;
} mplayer_logger;

typedef struct mplayer_desc {
    bool setup_called;
    mplayer_logger logger;
} mplayer_desc;

typedef struct mplayer_media {
    AVFormatContext *format_context;
    const AVCodec *video_codec;
    uint32_t video_stream_index;
    bool has_video_stream;
} mplayer_media;

typedef struct mplayer_pixel_data {
    uint8_t *buffer;
    int buffer_size;
    int width;
    int height;
    enum AVPixelFormat format;
} mplayer_pixel_data;

extern void mplayer_setup(const mplayer_desc *desc);

extern void mplayer_cleanup(void);

extern mplayer_media mplayer_open_file(const char *filename);

extern void mplayer_decode_media(const mplayer_media *media);

extern mplayer_pixel_data mplayer_alloc_pixel_data(void);

extern void mplayer_get_pixels(mplayer_pixel_data *data);

#endif
