#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"

#include "maker_internal.h"
#include "maker_player.h"

// >>structs
typedef struct {
    mplayer_desc desc;
    AVFrame *frame;
} _mplayer_state;

_MAKER_PRIVATE _mplayer_state _mplayer;

// >>logging
// #if defined(MAKER_DEBUG)
#define _MPLAYER_LOGITEM_XMACRO(item, msg) #item ": " msg,
_MAKER_PRIVATE const char *_mplayer_log_messages[] = {_MPLAYER_LOG_ITEMS};
#undef _MPLAYER_LOGITEM_XMACRO
// #endif // MAKER_DEBUG

// clang-format off
#define _MPLAYER_PANIC(code) _mplayer_log(MPLAYER_LOGITEM_ ##code, 0, 0, __LINE__)
#define _MPLAYER_ERROR(code) _mplayer_log(MPLAYER_LOGITEM_ ##code, 1, 0, __LINE__)
#define _MPLAYER_ERRORMSG(code,msg) _mplayer_log(MPLAYER_LOGITEM_ ##code, 1, msg, __LINE__)
#define _MPLAYER_WARN(code) _mplayer_log(MPLAYER_LOGITEM_ ##code, 2, 0, __LINE__)
#define _MPLAYER_WARNMSG(code,msg) _mplayer_log(MPLAYER_LOGITEM_ ##code, 2, msg, __LINE__)
#define _MPLAYER_INFO(code) _mplayer_log(MPLAYER_LOGITEM_ ##code, 3, 0, __LINE__)
// clang-format on

_MAKER_PRIVATE void _mplayer_log(mplayer_log_item log_item, uint32_t log_level,
                                 const char *msg, uint32_t line_nr) {
    if (_mplayer.desc.logger.func) {
        const char *filename = 0;
        // #if defined(MAKER_DEBUG)
        filename = __FILE__;
        if (0 == msg) {
            msg = _mplayer_log_messages[log_item];
        }
        // #endif
        _mplayer.desc.logger.func("mplayer", log_level, log_item, msg, line_nr,
                                  filename, _mplayer.desc.logger.user_data);
    } else {
        // for log level PANIC it would be 'undefined behaviour' to continue
        if (log_level == 0) {
            abort();
        }
    }
}

// >>debugging
_MAKER_PRIVATE void _mplayer_debug_stream(AVStream *stream) {
    fprintf(stdout, "AVStream->time_base before open coded %d/%d\n",
            stream->time_base.num, stream->time_base.den);
    fprintf(stdout, "AVStream->r_frame_rate before open coded %d/%d\n",
            stream->r_frame_rate.num, stream->r_frame_rate.den);
    fprintf(stdout, "AVStream->start_time %lld\n", stream->start_time);
    fprintf(stdout, "AVStream->duration %lld\n", stream->duration);
}

_MAKER_PRIVATE void _mplayer_debug_codec(const AVCodec *codec,
                                         AVStream *stream) {
    if (codec == NULL) {
        fprintf(stderr, "Couldn't find codec info");
        exit(1);
    }

    if (codec->type == AVMEDIA_TYPE_VIDEO) {
        AVCodecParameters *codec_parameters = stream->codecpar;
        fprintf(stdout, "Video Codec: resolution %d x %d\n",
                codec_parameters->width, codec_parameters->height);
    }
}

// >>save frame into file
// _MAKER_PRIVATE void _mplayer_save_gray_frame(unsigned char *buf, int wrap,
// int xsize,
//                                      int ysize, char *filename) {
//     FILE *f;
//     int i;
//     f = fopen(filename, "wb");
//     fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
//     for (i = 0; i < ysize; i++) {
//         fwrite(buf + i * wrap, 1, xsize, f);
//     }
//     fclose(f);
// }

_MAKER_PRIVATE void _mplayer_save_rgb_frame(unsigned char *buf, int wrap,
                                            int xsize, int ysize,
                                            char *filename) {
    FILE *f;
    int i;
    f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++) {
        fwrite(buf + i * wrap, 1, xsize * 3, f);
    }
    fclose(f);
}

// >>resources
_MAKER_PRIVATE AVFormatContext *_mplayer_alloc_format_context(void) {
    AVFormatContext *format_context = avformat_alloc_context();
    if (!format_context) {
        _MPLAYER_PANIC(AVFORMAT_ALLOC_FAILED);
    }
    return format_context;
}

_MAKER_PRIVATE AVFormatContext *_mplayer_open_file(const char *filename) {
    AVFormatContext *format_context = _mplayer_alloc_format_context();

    if (avformat_open_input(&format_context, filename, NULL, NULL) != 0) {
        _MPLAYER_PANIC(AVFORMAT_OPEN_FILE_FAILED);
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        _MPLAYER_PANIC(AVFORMAT_FIND_STREAM_INFO_FAILED);
    }

    return format_context;
}

_MAKER_PRIVATE AVCodecContext *
_mplayer_alloc_codec_context(const AVCodec *codec) {
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        _MPLAYER_PANIC(AVCODEC_ALLOC_CONTEXT_FAILED);
    }
    return codec_context;
}

_MAKER_PRIVATE AVFrame *_mplayer_alloc_frame(void) {
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        _MPLAYER_PANIC(AVUTIL_FRAME_ALLOC_FAILED);
    }
    return frame;
}

_MAKER_PRIVATE AVPacket *_mplayer_alloc_packet(void) {
    AVPacket *packet = av_packet_alloc();
    if (!packet) {
        _MPLAYER_PANIC(AVUTIL_PACKET_ALLOC_FAILED);
    }
    return packet;
}

_MAKER_PRIVATE int _mplayer_decode_packet(AVPacket *packet, AVFrame *frame,
                                          AVCodecContext *context) {
    char buffer[1024];

    int response = 0;
    response = avcodec_send_packet(context, packet);

    if (response < 0) {
        snprintf(buffer, sizeof(buffer),
                 "Error while sending a packet to the decoder: %s\n",
                 av_err2str(response));
        _MPLAYER_WARNMSG(AVCODEC_SEND_PACKET_FAILED, buffer);
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(context, frame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        }

        if (response < 0) {
            snprintf(buffer, sizeof(buffer),
                     "Error while receiving a frame from the decoder: %s\n",
                     av_err2str(response));
            _MPLAYER_WARNMSG(AVCODEC_RECEIVE_FRAME_FAILED, buffer);
            return response;
        }

        // enum AVPixelFormat fmt = AV_PIX_FMT_RGB24;
        enum AVPixelFormat fmt = AV_PIX_FMT_RGBA;

        uint32_t byteCount =
            av_image_get_buffer_size(fmt, frame->width, frame->height, 1);

        printf("sizeee %d %dx%d\n", byteCount, frame->width, frame->height);

        uint8_t *ibuffer = (uint8_t *)av_malloc(byteCount * sizeof(uint8_t));

        AVFrame *rgb_frame = _mplayer.frame;
        rgb_frame->width = context->width;
        rgb_frame->height = context->height;
        rgb_frame->format = fmt;

        av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, ibuffer, fmt,
                             context->width, context->height, 1);

        struct SwsContext *sws_context = sws_getContext(
            context->width, context->height, context->pix_fmt, context->width,
            context->height, fmt, SWS_BILINEAR, NULL, NULL, NULL);

        sws_scale(sws_context, (const uint8_t *const *)frame->data,
                  frame->linesize, 0, context->height, rgb_frame->data,
                  rgb_frame->linesize);

        printf("saving frame %3" PRId64 "\n", context->frame_num);
        fflush(stdout);

        snprintf(buffer, sizeof(buffer), "%s-%" PRId64 ".ppm", "output",
                 context->frame_num);
        // _mplayer_save_gray_frame(rgb_frame->data[0], rgb_frame->linesize[0],
        //                          context->width, context->height, buffer);
        _mplayer_save_rgb_frame(rgb_frame->data[0], rgb_frame->linesize[0],
                                context->width, context->height, buffer);

        av_free(ibuffer);
    }

    return 0;
}

// >>public
void mplayer_setup(const mplayer_desc *desc) {
    MAKER_ASSERT(!_mplayer.setup_called);
    MAKER_ASSERT(desc);
    _mplayer.desc = *desc;
    _mplayer.frame = _mplayer_alloc_frame();
}

void mplayer_cleanup(void) { av_frame_free(&_mplayer.frame); }

mplayer_media mplayer_open_file(const char *filename) {
    mplayer_media media;

    AVFormatContext *format_context = _mplayer_open_file(filename);
    media.format_context = format_context;

    printf("format %s, duration %lld us, bit_rate %lld\n",
           format_context->iformat->name, format_context->duration,
           format_context->bit_rate);

    for (unsigned int i = 0; i < format_context->nb_streams; i++) {
        AVStream *stream = format_context->streams[i];
        _mplayer_debug_stream(stream);

        AVCodecParameters *codec_parameters = stream->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
        if (codec == NULL) {
            _MPLAYER_PANIC(AVCODEC_FIND_CODEC_FAILED);
        }

        _mplayer_debug_codec(codec, stream);

        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            media.video_codec = codec;
            media.video_stream_index = i;
            media.has_video_stream = true;
        }
    }

    return media;
}

void mplayer_decode_media(const mplayer_media *media) {
    MAKER_ASSERT(media.has_video_stream);
    int err;

    AVCodecContext *codec_context =
        _mplayer_alloc_codec_context(media->video_codec);

    AVStream *stream =
        media->format_context->streams[media->video_stream_index];
    AVCodecParameters *codec_parameters = stream->codecpar;

    err = avcodec_parameters_to_context(codec_context, codec_parameters);
    if (err < 0) {
        _MPLAYER_ERROR(AVCODEC_COPY_PARAM_TO_CONTEXT_FAILED);
    }

    err = avcodec_open2(codec_context, media->video_codec, NULL);
    if (err < 0) {
        _MPLAYER_ERRORMSG(AVCODEC_OPEN_CODEC_FAILED, av_err2str(err));
        _MPLAYER_PANIC(AVCODEC_OPEN_CODEC_FAILED);
    }

    AVFrame *frame = _mplayer_alloc_frame();
    AVPacket *packet = _mplayer_alloc_packet();

    int response = 0;
    int packet_count = 1;
    while (av_read_frame(media->format_context, packet) >= 0) {
        if (packet->stream_index == (int)media->video_stream_index) {
            response = _mplayer_decode_packet(packet, frame, codec_context);
            if (response < 0) {
                break;
            }
            if (--packet_count <= 0)
                break;
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);
}

mplayer_pixel_data mplayer_alloc_pixel_data(void) {
    MAKER_ASSERT(_mplayer.frame);
    int size =
        av_image_get_buffer_size(_mplayer.frame->format, _mplayer.frame->width,
                                 _mplayer.frame->height, 1);
    mplayer_pixel_data data;
    data.buffer = malloc(size * sizeof(uint8_t));
    data.buffer_size = size;
    data.width = _mplayer.frame->width;
    data.height = _mplayer.frame->height;
    data.format = _mplayer.frame->format;
    return data;
}

void mplayer_free_pixel_data(mplayer_pixel_data *data) { free(data->buffer); }

void mplayer_get_pixels(mplayer_pixel_data *data) {
    MAKER_ASSERT(_mplayer.frame);

    int ret = av_image_copy_to_buffer(
        data->buffer, data->buffer_size,
        (const uint8_t *const *)_mplayer.frame->data, _mplayer.frame->linesize,
        data->format, data->width, data->height, 1);

    if (ret < 0) {
        char str[512];
        maker_strfmt(str, MAKER_LEN(str), "%s", av_err2str(ret));
        _MPLAYER_ERRORMSG(AV_IMAGE_COPY_TO_BUFFER_FAILED, str);
        _MPLAYER_PANIC(AV_IMAGE_COPY_TO_BUFFER_FAILED);
    }
}
