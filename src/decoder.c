#include "internal.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavcodec/codec_par.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
#include "maker/decoder.h"
#include "maker/format.h"
#include "maker/media.h"
#include "maker/track.h"
#include <stdint.h>
#include <stdio.h>

_MK_PRIVATE int mk_decoder_get_pixel(MKDecoder* decoder, MKImageData* target)
{
    int err;
    AVCodecContext* codec_context = decoder->video_decoder.codec_context;
    enum AVPixelFormat target_format
        = mk_format_to_av_pixel_format(target->format);

    if (!decoder->video_decoder.sws_context) {
        struct SwsContext* sws_context = sws_getContext(
            codec_context->width, codec_context->height, codec_context->pix_fmt,
            codec_context->width, codec_context->height, target_format,
            SWS_BILINEAR, NULL, NULL, NULL
        );
        MK_ASSERT(sws_context);
        decoder->video_decoder.sws_context = sws_context;
    }

    struct SwsContext* sws_context = decoder->video_decoder.sws_context;
    AVFrame* src_frame = decoder->video_decoder.frame;

    if (codec_context->width != target->width
        || codec_context->height != target->height) {
        fprintf(
            stderr, "MKImageData does not match AVCodecContext dimensions\n"
        );
        return -1;
    }

    AVFrame* dst_frame = av_frame_alloc();
    err = av_image_alloc(
        dst_frame->data, dst_frame->linesize, codec_context->width,
        codec_context->height, target_format, 1
    );

    if (err < 0) {
        fprintf(
            stderr, "Failed to allocate image. Cause: %s\n", av_err2str(err)
        );
        return -1;
    }

    sws_scale(
        sws_context, (const uint8_t* const*)src_frame->data,
        src_frame->linesize, 0, codec_context->height, dst_frame->data,
        dst_frame->linesize
    );

    err = av_image_copy_to_buffer(
        target->buffer, target->buffer_size,
        (const uint8_t* const*)dst_frame->data, dst_frame->linesize,
        target_format, target->width, target->height, 1
    );

    av_frame_free(&dst_frame);

    if (err < 0) {
        fprintf(
            stderr, "Failed to copy image to buffer. Cause: %s\n",
            av_err2str(err)
        );
        return -1;
    }

    return 0;
}

int mk_decoder_start(MKDecoder* decoder, MKMedia* media)
{
    MK_ASSERT(decoder);
    MK_ASSERT(media);

    MKTrack video_track
        = mk_media_get_track_with_type(media, MKTRACK_TYPE_VIDEO);
    if (video_track.is_valid) {
        AVStream* stream
            = media->format_context->streams[video_track.stream_index];
        AVCodecParameters* params = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(params->codec_id);

        if (!codec) {
            return -1;
        }

        AVCodecContext* ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, params);
        avcodec_open2(ctx, codec, NULL);

        AVFrame* frame = av_frame_alloc();
        if (!frame) {
            return -1;
        }

        decoder->video_decoder = (MKVideoDecoder) {
            .codec_context = ctx,
            .frame = frame,
            .is_valid = 1,
        };
    }

    MKTrack audio_track
        = mk_media_get_track_with_type(media, MKTRACK_TYPE_AUDIO);
    if (audio_track.is_valid) {
        AVStream* stream
            = media->format_context->streams[audio_track.stream_index];
        AVCodecParameters* params = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(params->codec_id);

        if (!codec) {
            return -1;
        }

        AVCodecContext* ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, params);
        avcodec_open2(ctx, codec, NULL);

        decoder->audio_decoder = (MKAudioDecoder) {
            .codec_context = ctx,
            .is_valid = 1,
        };
    }

    decoder->is_valid = 1;
    decoder->media = media;

    return 0;
}

void mk_decoder_stop(MKDecoder* decoder)
{
    MK_ASSERT(decoder);

    if (decoder->video_decoder.codec_context) {
        avcodec_free_context(&decoder->video_decoder.codec_context);
        av_frame_free(&decoder->video_decoder.frame);
        decoder->video_decoder.is_valid = 0;
    }

    if (decoder->audio_decoder.codec_context) {
        avcodec_free_context(&decoder->audio_decoder.codec_context);
        decoder->audio_decoder.is_valid = 0;
    }

    decoder->is_valid = 0;
}

_MK_PRIVATE int
mk_decoder_read_video_frame(MKDecoder* decoder, AVPacket* packet)
{
    int err;
    AVCodecContext* ctx = decoder->video_decoder.codec_context;
    err = avcodec_send_packet(ctx, packet);
    if (err != 0) {
        fprintf(stderr, "Failed to send packet. Cause: %s\n", av_err2str(err));
        return -1;
    }

    err = avcodec_receive_frame(ctx, decoder->video_decoder.frame);
    if (err != 0) {
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            return err;
        }

        fprintf(
            stderr,
            "Error while receiving a frame from the decoder. Cause: %s\n",
            av_err2str(err)
        );

        return -1;
    }

    return 0;
}

int mk_decoder_read_frame(MKDecoder* decoder, MKImageData* target)
{
    MK_ASSERT(decoder);
    MK_ASSERT(target);

    int err;
    AVPacket* packet = av_packet_alloc();

    MKTrack video_track
        = mk_media_get_track_with_type(decoder->media, MKTRACK_TYPE_VIDEO);

    for (;;) {
        err = av_read_frame(decoder->media->format_context, packet);
        if (err != 0) {
            break;
        }

        if (video_track.is_valid
            && video_track.stream_index == packet->stream_index) {
            err = mk_decoder_read_video_frame(decoder, packet);
            if (err == 0) {
                err = mk_decoder_get_pixel(decoder, target);
                break; // take a single frame
            }
        }
    }

    av_packet_free(&packet);

    return err;
}
