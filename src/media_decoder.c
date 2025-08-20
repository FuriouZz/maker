#include "decoder.h"
#include "format.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavcodec/codec_par.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
#include "maker/maker.h"
#include "media.h"
#include "media_decoder.h"
#include "mutex.h"
#include "thread.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

_MK_PRIVATE int mk_decoder_get_pixel(
    MKMediaDecoder* decoder, MKImageData* target, AVFrame* src_frame
)
{
    int err;
    AVCodecContext* codec_context
        = decoder->video_decoder.decoder.codec_context;
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

int mk_media_decoder_init(MKMediaDecoder* decoder, MKMedia* media)
{
    MK_ASSERT(decoder);
    MK_ASSERT(media);

    int ret;

    ret = mk_packet_queue_init(&decoder->packet_queue);
    if (ret != 0) {
        return ret;
    }

    ret = mk_frame_queue_init(
        &decoder->frame_queue, &decoder->packet_queue, 16, 1
    );
    if (ret != 0) {
        return ret;
    }

    ret = mk_cond_init(&decoder->is_empty_signal);
    if (ret != 0) {
        return ret;
    }

    MKTrack video_track = { 0 };
    if (mk_media_get_track_from_type(&video_track, media, MKTRACK_TYPE_VIDEO)
        == 0) {
        AVStream* stream
            = media->context->format->streams[video_track.stream_index];
        AVCodecParameters* params = stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(params->codec_id);

        if (!codec) {
            return -1;
        }

        AVCodecContext* ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, params);
        avcodec_open2(ctx, codec, NULL);

        decoder->video_decoder.is_valid = 1;
        decoder->video_stream_index = video_track.stream_index;

        mk_decoder_init(
            &decoder->video_decoder.decoder, ctx, &decoder->packet_queue,
            &decoder->is_empty_signal
        );
    }

    MKTrack audio_track = { 0 };
    if (mk_media_get_track_from_type(&audio_track, media, MKTRACK_TYPE_AUDIO)
        == 0) {
        AVStream* stream
            = media->context->format->streams[audio_track.stream_index];
        AVCodecParameters* params = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(params->codec_id);

        if (!codec) {
            return -1;
        }

        AVCodecContext* ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(ctx, params);
        avcodec_open2(ctx, codec, NULL);

        decoder->audio_decoder.is_valid = 1;
        decoder->audio_stream_index = audio_track.stream_index;
    }

    decoder->is_aborted = 1;
    decoder->is_valid = 1;
    decoder->media = media;
    mk_cond_init(&decoder->demuxer.continue_signal);

    return 0;
}

_MK_PRIVATE int demuxer_thread(void* data)
{
    int ret;
    MKMediaDecoder* decoder = data;
    MKDemuxer* demuxer = &decoder->demuxer;
    AVFormatContext* format_context = decoder->media->context->format;
    MKDecoder* video_decoder = &decoder->video_decoder.decoder;

    MKMutex wait_mutex;
    ret = mk_mutex_init(&wait_mutex);
    if (ret != 0) {
        return AVERROR(ret);
    }

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        return AVERROR(ENOMEM);
    }

    for (;;) {
        if (decoder->is_aborted) {
            break;
        }

        // Is it full?
        if (video_decoder->packet_queue->packet_count > MAX_QUEUE_SIZE) {
            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(&demuxer->continue_signal, &wait_mutex, 10);
            mk_mutex_unlock(&wait_mutex);
            continue;
        }

        ret = av_read_frame(format_context, packet);

        if (ret < 0) {
            if (ret == AVERROR_EOF && !demuxer->is_eof) {
                // printf("read ret=%d==%d\n", ret, AVERROR_EOF);
                demuxer->is_eof = 1;
            }

            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(&demuxer->continue_signal, &wait_mutex, 10);
            mk_mutex_unlock(&wait_mutex);
            continue;
        } else {
            demuxer->is_eof = 0;
        }

        if (packet->stream_index == decoder->video_stream_index) {
            printf("put count=%d\n", video_decoder->packet_queue->packet_count);
            mk_packet_queue_put(video_decoder->packet_queue, packet);
        } else {
            av_packet_unref(packet);
        }
    }

    ret = 0;

    av_packet_free(&packet);
    mk_mutex_destroy(&wait_mutex);

    return 0;
}

int decode_frame(MKMediaDecoder* decoder, AVFrame* frame)
{
    int ret = AVERROR(EAGAIN);
    MKDecoder* dec = &decoder->video_decoder.decoder;
    AVCodecContext* codec_context
        = decoder->video_decoder.decoder.codec_context;

    for (;;) {
        if (dec->packet_queue->serial == dec->packet.serial) {
            do {
                if (dec->packet_queue->is_aborted) {
                    return -1;
                }
                ret = avcodec_receive_frame(codec_context, frame);
                if (ret == AVERROR_EOF) {
                    avcodec_flush_buffers(codec_context);
                    return 0;
                }

                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));
        }

        for (;;) {
            printf("size????? %d\n", dec->packet_queue->packet_count);

            if (dec->packet_queue->packet_count == 0) {
                mk_cond_signal(&decoder->is_empty_signal);
            }

            if (dec->has_packet_pending) {
                dec->has_packet_pending = 0;
            } else {
                int old_serial = dec->packet.serial;

                int retval = mk_packet_queue_get(
                    dec->packet_queue, dec->packet.packet, 1,
                    &dec->packet.serial
                );
                if (retval < 0) {
                    return -1;
                }

                printf(
                    "old_serial=%d pkt_serial=%d\n", old_serial,
                    dec->packet.serial
                );
                if (old_serial != dec->packet.serial) {
                    avcodec_flush_buffers(codec_context);
                }
            }

            if (dec->packet_queue->serial == dec->packet.serial) {
                break;
            }

            av_packet_unref(dec->packet.packet);
        }

        printf("send packet\n");
        if (avcodec_send_packet(codec_context, dec->packet.packet)
            == AVERROR(EAGAIN)) {
            dec->has_packet_pending = 1;
        } else {
            av_packet_unref(dec->packet.packet);
        }
    }

    return 0;
}

_MK_PRIVATE int decode_video_thread(void* data)
{
    int ret;
    MKMediaDecoder* decoder = data;

    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        return AVERROR(ENOMEM);
    }

    for (;;) {
        ret = decode_frame(decoder, frame);
        printf("decode %lld ret=%d\n", frame->pts, ret);

        if (ret < 0) {
            goto the_end;
        }

        if (ret == 0) {
            continue;
        }

        // while (ret >= 0) {
        printf("peek?\n");
        MKFrameQueueItem* item
            = mk_frame_queue_peek_writable(&decoder->frame_queue);

        if (item == NULL) {
            ret = -1;
            break;
        }

        item->serial = decoder->video_decoder.decoder.packet.serial;

        av_frame_move_ref(item->frame, frame);
        mk_frame_queue_push(&decoder->frame_queue);
        ret = 0;

        av_frame_unref(frame);

        printf(
            "%d==%d ret=%d\n", decoder->video_decoder.decoder.packet.serial,
            decoder->video_decoder.decoder.packet_queue->serial, ret
        );

        //     if (decoder->video_decoder.decoder.packet.serial
        //         != decoder->video_decoder.decoder.packet_queue->serial) {
        //         break;
        //     }
        // }

        if (ret < 0) {
            goto the_end;
        }
    }

the_end:
    av_frame_free(&frame);

    return 0;
}

void mk_media_decoder_start(MKMediaDecoder* decoder)
{
    decoder->is_aborted = 0;

    // Start demuxer
    decoder->demuxer.thread.name = "demuxer_thread";
    decoder->demuxer.thread.fn = demuxer_thread;
    decoder->demuxer.thread.userdata = decoder;
    mk_thread_init(&decoder->demuxer.thread);

    // Start video decoder
    if (decoder->video_decoder.is_valid) {
        MKDecoder* dec = &decoder->video_decoder.decoder;
        dec->thread.name = "decode_video_thread";
        dec->thread.fn = decode_video_thread;
        dec->thread.userdata = decoder;
        mk_decoder_start(dec);
    }
}

void mk_media_decoder_stop(MKMediaDecoder* decoder)
{
    decoder->is_aborted = 1;
    mk_packet_queue_abort(&decoder->packet_queue);
    mk_thread_wait(&decoder->demuxer.thread, NULL);

    if (decoder->video_decoder.is_valid) {
        MKDecoder* dec = &decoder->video_decoder.decoder;
        mk_decoder_abort(dec, &decoder->frame_queue);
    }
}

void mk_media_decoder_free(MKMediaDecoder* decoder)
{
    MK_ASSERT(decoder);

    if (decoder->video_decoder.is_valid) {
        mk_decoder_destroy(&decoder->video_decoder.decoder);
        decoder->video_decoder.is_valid = 0;
    }

    if (decoder->audio_decoder.is_valid) {
        decoder->audio_decoder.is_valid = 0;
    }

    mk_frame_queue_destroy(&decoder->frame_queue);
    mk_packet_queue_destroy(&decoder->packet_queue);
    mk_cond_destroy(&decoder->is_empty_signal);

    decoder->is_valid = 0;
}

_MK_PRIVATE int mk_media_decoder_read_video_frame(
    MKMediaDecoder* decoder, AVPacket* packet, AVFrame* frame
)
{
    int err;
    AVCodecContext* ctx = decoder->video_decoder.decoder.codec_context;

    err = avcodec_send_packet(ctx, packet);
    if (err != 0) {
        fprintf(stderr, "Failed to send packet. Cause: %s\n", av_err2str(err));
        return -1;
    }

    err = avcodec_receive_frame(ctx, frame);
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

int mk_media_decoder_read_frame(MKMediaDecoder* decoder, MKImageData* target)
{
    MK_ASSERT(decoder);
    MK_ASSERT(target);

    int err;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    MKTrack video_track = { 0 };
    err = mk_media_get_track_from_type(
        &video_track, decoder->media, MKTRACK_TYPE_VIDEO
    );
    if (err != 0) {
        printf("No video track found");
        return -1;
    }

    for (;;) {
        err = av_read_frame(decoder->media->context->format, packet);
        if (err != 0) {
            break;
        }

        if (video_track.stream_index == packet->stream_index) {
            err = mk_media_decoder_read_video_frame(decoder, packet, frame);
            if (err == 0) {
                err = mk_decoder_get_pixel(decoder, target, frame);
                break; // take a single frame
            }
        }
    }

    av_packet_free(&packet);

    return err;
}

int mk_media_decoder_read_frame2(MKMediaDecoder* decoder, MKImageData* target)
{
    MK_ASSERT(decoder);
    MK_ASSERT(target);

    int frame_count
        = mk_frame_queue_remaining_frame_count(&decoder->frame_queue);

    printf("frame_count=%d\n", frame_count);

    // retry:
    // if (frame_count > 0) {

    //     MKFFMpegFrameQueueItem* last
    //         = mk_ffmpeg_frame_queue_peek_last(&decoder->frame_queue);
    MKFrameQueueItem* item = mk_frame_queue_peek(&decoder->frame_queue);
    if (item == NULL) {
        return -1;
    }

    // printf(
    //     "%d %d\n", item->serial, decoder->video_decoder.packet_queue.serial
    // );

    // if (item->serial != decoder->video_decoder.packet_queue.serial) {
    //     mk_ffmpeg_frame_queue_next(&decoder->frame_queue);
    //     goto retry;
    // }

    // printf("width=%d, pts=%lld\n", item->frame->width, item->frame->pts);

    mk_decoder_get_pixel(decoder, target, item->frame);
    mk_frame_queue_next(&decoder->frame_queue);
    // }
    return 0;
}
