#ifndef MAKER_DECODER_H
#define MAKER_DECODER_H

#include "stdbool.h"

#include "libavcodec/codec.h"
#include "libavformat/avformat.h"

// clang-format off
#define _MDECODER_LOG_ITEMS \
    _MDECODER_LOGITEM_XMACRO(OK, "Ok") \
    _MDECODER_LOGITEM_XMACRO(AVFORMAT_ALLOC_FAILED, "Could not allocate memory for AVFormatContext") \
    _MDECODER_LOGITEM_XMACRO(AVFORMAT_OPEN_FILE_FAILED, "Could not open file") \
    _MDECODER_LOGITEM_XMACRO(AVFORMAT_FIND_STREAM_INFO_FAILED, "Could not open file") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_FIND_CODEC_FAILED, "Could not find codec") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_ALLOC_CONTEXT_FAILED, "Could not allocate memory for AVCodecContext") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_OPEN_CODEC_FAILED, "Could not open codec") \
    _MDECODER_LOGITEM_XMACRO(AVUTIL_FRAME_ALLOC_FAILED, "Could not allocate memory for AVFrame") \
    _MDECODER_LOGITEM_XMACRO(AVUTIL_PACKET_ALLOC_FAILED, "Could not allocate memory for AVPacket") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_SEND_PACKET_FAILED, "Error while sending a packet to the decoder") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_RECEIVE_FRAME_FAILED, "Error while receiving a frame from the decoder") \
    _MDECODER_LOGITEM_XMACRO(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED, "Failed to copy codec params to codec context") \
    _MDECODER_LOGITEM_XMACRO(AV_IMAGE_COPY_TO_BUFFER_FAILED, "Failed to copy AVFrame to buffer") \

#define _MDECODER_LOGITEM_XMACRO(item, msg) MDECODER_LOGITEM_##item,
typedef enum mdecoder_log_item { _MDECODER_LOG_ITEMS } mdecoder_log_item;
#undef _MDECODER_LOGITEM_XMACRO
// clang-format on

typedef struct mdecoder_logger {
    void (*func)(const char *tag,      // always "mdecoder"
                 uint32_t log_level,   // 0=panic, 1=error, 2=warning, 3=info
                 uint32_t log_item_id, // MDECODER_LOGITEM_*
                 const char *message_or_null,  // a message string, may be
                                               // nullptr in release mode
                 uint32_t line_nr,             // line number in video_player.h
                 const char *filename_or_null, // source filename, may be
                                               // nullptr in release mode
                 void *user_data);
    void *user_data;
} mdecoder_logger;

typedef struct mdecoder_desc {
    bool setup_called;
    mdecoder_logger logger;
} mdecoder_desc;

typedef struct mdecoder_media {
    const char *filename;
    AVFormatContext *format_context;
    struct {
        bool has_stream;
        int stream_index;
        int width;
        int height;
        const AVCodec *codec;
    } video;
    struct {
        bool has_stream;
        int stream_index;
        const AVCodec *codec;
    } audio;
} mdecoder_media;

typedef struct mdecoder_image_data {
    uint8_t *buffer;
    int buffer_size;
    int width;
    int height;
    enum AVPixelFormat format;
} mdecoder_image_data;

extern void mdecoder_setup(const mdecoder_desc *desc);

extern void mdecoder_cleanup(void);

extern mdecoder_media mdecoder_create_media(const char *filename);

extern void mdecoder_decode_media(const mdecoder_media *media);

extern mdecoder_image_data
mdecoder_alloc_image_data(const mdecoder_media *media);

extern void mdecoder_get_pixels(const mdecoder_image_data *data);

extern void mdecoder_free_pixel_data(const mdecoder_image_data *data);

#endif
