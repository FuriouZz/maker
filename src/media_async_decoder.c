#include "clock.h"
#include "decoder.h"
#include "format.h"
#include "frame_queue.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/codec.h"
#include "libavcodec/codec_par.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"
#include "libavutil/pixfmt.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
#include "maker/maker.h"
#include "media.h"
#include "media_async_decoder.h"
#include "mutex.h"
#include "packet_queue.h"
#include "thread.h"
#include "util.h"
#include <math.h>
#include <stdint.h>

#define MAX_QUEUE_ITEM 25

int mk_media_async_decoder_init(
    MKMediaAsyncDecoder* decoder, MKMediaAsyncDecoderDesc* desc
)
{
    MK_ASSERT(decoder);
    MK_ASSERT(desc);

    MKMedia* media = desc->media;
    MK_ASSERT(desc->media);

    mk_clear(decoder, sizeof(MKMediaAsyncDecoder));

    decoder->media = desc->media;
    decoder->converter.target_format = AV_PIX_FMT_RGBA;
    decoder->max_frame_duration
        = desc->media->context->format->flags & AVFMT_TS_DISCONT ? 10.0
                                                                 : 3600.0;

    MKDecoder* video_decoder = &decoder->video_decoder;
    MKPacketQueue* packet_queue = &decoder->packet_queue;
    MKFrameQueue* picture_queue = &decoder->picture_queue;
    MKCond* continue_demux_signal = &decoder->continue_demux_signal;
    MKClock* clock = &decoder->clock;
    int video_stream_index = media->streams[MKTRACK_TYPE_VIDEO];

    if (mk_packet_queue_init(packet_queue) != 0) {
        printf("Failed to initialize packet queue\n");
        goto fail;
    }

    if (mk_frame_queue_init(picture_queue, packet_queue, 16, 1) != 0) {
        printf("Failed to initialize frame queue\n");
        goto fail;
    }

    if (mk_cond_init(continue_demux_signal) != 0) {
        printf("Failed to initialize signal\n");
        goto fail;
    }

    if (mk_clock_init(clock) != 0) {
        printf("Failed to initialize clock");
        goto fail;
    }

    if (video_stream_index > -1) {
        AVFormatContext* format = media->context->format;
        AVStream* stream = format->streams[video_stream_index];
        AVCodecParameters* params = stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(params->codec_id);
        if (codec == NULL) {
            printf("Failed to find codec\n");
            goto fail;
        }

        AVCodecContext* codec_context = avcodec_alloc_context3(codec);
        if (codec_context == NULL) {
            printf("Failed to find allocate codec context\n");
            goto fail;
        }

        if (avcodec_parameters_to_context(codec_context, params) != 0) {
            printf("Failed to transfer parameters to codec context\n");
            goto fail;
        }

        if (avcodec_open2(codec_context, codec, NULL) != 0) {
            printf("Failed to open codec\n");
            goto fail;
        }

        if (mk_decoder_init(
                video_decoder, codec_context, packet_queue,
                continue_demux_signal
            )
            != 0) {
            printf("Failed to initialize video decoder\n");
            goto fail;
        }
    }

    return 0;

fail:
    mk_clock_free(clock);
    mk_packet_queue_destroy(packet_queue);
    mk_frame_queue_destroy(picture_queue);
    mk_cond_destroy(continue_demux_signal);
    mk_decoder_destroy(video_decoder);
    decoder->media = NULL;

    return -1;
}

void mk_media_async_decoder_destroy(MKMediaAsyncDecoder* decoder)
{
    if (decoder != NULL) {
        mk_packet_queue_destroy(&decoder->packet_queue);
        mk_frame_queue_destroy(&decoder->picture_queue);
        mk_cond_destroy(&decoder->continue_demux_signal);
        mk_decoder_destroy(&decoder->video_decoder);

        av_frame_free(&decoder->converter.frame);
        sws_freeContext(decoder->converter.sws_context);
    }
}

_MK_PRIVATE int mk_media_async_demuxer_thread(void* data)
{
    int ret;
    MKMediaAsyncDecoder* decoder = data;
    MKMedia* media = decoder->media;
    MKPacketQueue* queue = &decoder->packet_queue;
    AVFormatContext* format = decoder->media->context->format;

    AVPacket* packet = av_packet_alloc();
    if (packet == NULL) {
        printf("Failed to allocate packet\n");
        goto the_end;
    }

    MKMutex wait_mutex;
    if (mk_mutex_init(&wait_mutex) != 0) {
        printf("Failed to initialize mutex\n");
        goto the_end;
    }

    for (;;) {
        if (decoder->is_aborted) {
            ret = 0;
            break;
        }

        if (queue->packet_count >= MAX_QUEUE_ITEM) {
            printf("Queue is full.\n");
            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(&decoder->continue_demux_signal, &wait_mutex, 10);
            mk_mutex_unlock(&wait_mutex);
            continue;
        }

        ret = av_read_frame(format, packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF && !decoder->is_eof) {
                decoder->is_eof = 1;
            }

            mk_mutex_lock(&wait_mutex);
            mk_cond_timedwait(&decoder->continue_demux_signal, &wait_mutex, 10);
            mk_mutex_unlock(&wait_mutex);
            continue;
        } else {
            decoder->is_eof = 0;
        }

        if (packet->stream_index == media->streams[MKTRACK_TYPE_VIDEO]) {
            printf(
                "packet - pts=%lld dts=%lld pos=%lld\n", packet->pts,
                packet->dts, packet->pos
            );
            mk_packet_queue_put(queue, packet);
        } else {
            av_packet_unref(packet);
        }
    }

the_end:
    mk_mutex_destroy(&wait_mutex);
    av_packet_free(&packet);

    return 0;
}

/**
 * Decoding API
 * https://ffmpeg.org/doxygen/4.0/group__lavc__encdec.html
 */
_MK_PRIVATE int
mk_media_async_decoder_get_video_frame(MKDecoder* decoder, AVFrame* frame)
{
    int ret = AVERROR(EAGAIN);
    AVCodecContext* context = decoder->codec_context;
    MKPacketQueue* packet_queue = decoder->packet_queue;

    for (;;) {
        do {
            if (packet_queue->is_aborted) {
                return -1;
            }

            ret = avcodec_receive_frame(context, frame);
            if (ret == AVERROR_EOF) {
                decoder->is_finished = 1;
                avcodec_flush_buffers(context);
                return 0;
            }

            if (ret >= 0) {
                return 1;
            }
        } while (ret != AVERROR(EAGAIN));

        for (;;) {
            if (packet_queue->packet_count == 0) {
                mk_cond_signal(decoder->is_empty_signal);
            }

            ret = mk_packet_queue_get(
                packet_queue, decoder->packet.packet, 1, NULL
            );

            if (ret < 0) {
                return -1;
            } else if (ret == 1) {
                break;
            }

            av_packet_unref(decoder->packet.packet);
        }

        if (avcodec_send_packet(context, decoder->packet.packet)
            == AVERROR(EAGAIN)) {
            printf(
                "receive_frame and send_packet both returned EAGAIN, which is "
                "an API violation.\n "
            );
            return -1;
        }

        av_packet_unref(decoder->packet.packet);
    }

    return 0;
}

_MK_PRIVATE int mk_media_async_decoder_video_thread(void* data)
{
    int ret;
    MKMediaAsyncDecoder* decoder = data;
    MKFrameQueue* picture_queue = &decoder->picture_queue;
    MKDecoder* video_decoder = &decoder->video_decoder;

    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        printf("Failed to allocate frame\n");
        goto the_end;
    }

    for (;;) {
        ret = mk_media_async_decoder_get_video_frame(video_decoder, frame);

        if (ret < 0) {
            goto the_end;
        }

        if (ret == 0) {
            continue;
        }

        MKFrameQueueItem* item = mk_frame_queue_peek_writable(picture_queue);
        if (item == NULL) {
            break;
        }

        printf(
            "frame - pts=%lld best_effort_timestamp=%lld dts=%lld\n",
            frame->pts, frame->best_effort_timestamp, frame->pkt_dts
        );

        item->width = frame->width;
        item->height = frame->height;
        item->format = frame->format;
        item->pts = frame->best_effort_timestamp;
        item->duration = frame->duration;
        av_frame_move_ref(item->frame, frame);
        av_frame_unref(frame);
        mk_frame_queue_push(picture_queue);
    }

the_end:
    av_frame_free(&frame);

    return 0;
}

int mk_media_async_decoder_start(MKMediaAsyncDecoder* decoder)
{
    MK_ASSERT(decoder);
    int ret;

    MKMedia* media = decoder->media;
    MKThread* demuxer_thread = &decoder->demuxer_thread;
    MKPacketQueue* queue = &decoder->packet_queue;
    MKClock* clock = &decoder->clock;

    decoder->is_aborted = 0;
    mk_packet_queue_start(queue);
    mk_clock_start(clock);

    demuxer_thread->name = "demuxer_thread";
    demuxer_thread->fn = mk_media_async_demuxer_thread;
    demuxer_thread->userdata = decoder;
    ret = mk_thread_init(demuxer_thread);
    if (ret != 0) {
        printf("Failed to initialize demuxer thread\n");
        return -1;
    }

    if (media->streams[MKTRACK_TYPE_VIDEO] > -1) {
        MKDecoder* video_decoder = &decoder->video_decoder;
        MKThread* video_thread = &video_decoder->thread;
        video_thread->name = "video_thread";
        video_thread->fn = mk_media_async_decoder_video_thread;
        video_thread->userdata = decoder;
        ret = mk_thread_init(video_thread);
        if (ret != 0) {
            printf("Failed to initialize video thread\n");
            return -1;
        }
    }

    return 0;
}

void mk_media_async_decoder_stop(MKMediaAsyncDecoder* decoder)
{
    MK_ASSERT(decoder);

    MKMedia* media = decoder->media;
    MKThread* demuxer_thread = &decoder->demuxer_thread;
    MKPacketQueue* queue = &decoder->packet_queue;
    MKFrameQueue* picture_queue = &decoder->picture_queue;
    MKClock* clock = &decoder->clock;

    decoder->is_aborted = 1;
    mk_cond_signal(
        &decoder->continue_demux_signal
    ); // force demuxer to cancel sooner
    mk_thread_wait(demuxer_thread, NULL);
    mk_packet_queue_abort(queue);
    mk_clock_pause(clock);

    if (media->streams[MKTRACK_TYPE_VIDEO] > -1) {
        MKDecoder* video_decoder = &decoder->video_decoder;
        mk_decoder_abort(video_decoder, picture_queue);
    }
}

_MK_PRIVATE int
mk_media_async_decoder_yuv2rgb(MKMediaAsyncDecoder* decoder, AVFrame* src_frame)
{
    int ret;
    MKMediaConverter* converter = &decoder->converter;
    int width = decoder->video_decoder.codec_context->width;
    int height = decoder->video_decoder.codec_context->height;
    int src_format = decoder->video_decoder.codec_context->pix_fmt;
    enum AVPixelFormat dst_format = converter->target_format;

    if (converter->sws_context == NULL) {
        converter->sws_context = sws_getContext(
            width, height, src_format, width, height, dst_format, SWS_BILINEAR,
            NULL, NULL, NULL
        );
    }

    if (converter->frame == NULL) {
        converter->frame = av_frame_alloc();
        ret = av_image_alloc(
            converter->frame->data, converter->frame->linesize, width, height,
            decoder->converter.target_format, 1
        );
    }

    AVFrame* dst_frame = converter->frame;

    ret = sws_scale(
        converter->sws_context, (const uint8_t* const*)src_frame->data,
        src_frame->linesize, 0, src_frame->height, dst_frame->data,
        dst_frame->linesize
    );

    if (ret == 0) {
        return -1;
    }

    return 0;
}

int mk_media_async_decoder_get_picture(
    MKMediaAsyncDecoder* decoder, MKImageData* target
)
{
    int ret;
    MKFrameQueue* picture_queue = &decoder->picture_queue;

    MKFrameQueueItem* item = mk_frame_queue_peek(picture_queue);
    if (item == NULL) {
        printf("No picture to collect\n");
        return -1;
    }
    mk_frame_queue_next(picture_queue);

    ret = mk_image_data_init(
        target,
        &(MKImageDataDesc) {
            .width = item->width,
            .height = item->height,
            .format
            = mk_format_from_av_pixel_format(decoder->converter.target_format),
        }
    );

    if (ret != 0) {
        printf("Failed to initialize image data\n");
        return -1;
    }

    if (mk_media_async_decoder_yuv2rgb(decoder, item->frame)) {
        printf("Failed to convert\n");
        return -1;
    }

    ret = av_image_copy_to_buffer(
        target->buffer, target->buffer_size,
        (const uint8_t* const*)decoder->converter.frame->data,
        decoder->converter.frame->linesize, decoder->converter.target_format,
        target->width, target->height, 1
    );

    if (ret < 0) {
        printf("Failed to copy image data\n");
        return -1;
    }

    return 0;
}

int mk_media_decoder_get_playback_time(
    MKMediaAsyncDecoder* decoder, int* time_ms
)
{
    MK_CHECK_VALID(decoder);
    MK_CHECK_VALID(time_ms);

    int ret;
    MKClock* clock = &decoder->clock;

    MKTime time;
    ret = mk_get_time(&time);
    MK_CHECK_RESULT(ret);

    *time_ms = (time.tv_sec - clock->start_time->tv_sec) * 1000
        + (time.tv_nsec - clock->start_time->tv_nsec) / 1000000;

    return 0;
}

int mk_media_async_refresh(MKMediaAsyncDecoder* decoder)
{
    MK_CHECK_VALID(decoder);

    int ret;

    MKFrameQueue* picture_queue = &decoder->picture_queue;
    // MKPacketQueue* video_queue = decoder->video_decoder.packet_queue;

    int time_spent;
    ret = mk_media_decoder_get_playback_time(decoder, &time_spent);
    MK_CHECK_RESULT(ret);

retry:
    if (mk_frame_queue_remaining_frame_count(picture_queue) == 0) {
        goto retry;
    } else {
        MKFrameQueueItem* next = mk_frame_queue_peek_next(picture_queue);

        if (next->pts <= time_spent) {
            goto retry;
        }
    }

    return 0;

    //     MK_ASSERT(decoder);

    //     double time;
    //     double last_duration;
    //     double delay;

    //     MKFrameQueue* picture_queue = &decoder->picture_queue;
    //     MKPacketQueue* video_queue = decoder->video_decoder.packet_queue;

    // retry:
    //     if (mk_frame_queue_remaining_frame_count(picture_queue) == 0) {
    //         return 0;
    //     }

    //     MKFrameQueueItem* item = mk_frame_queue_peek(picture_queue);
    //     MKFrameQueueItem* last_item =
    //     mk_frame_queue_peek_last(picture_queue);

    //     if (item->serial != video_queue->serial) {
    //         mk_frame_queue_next(picture_queue);
    //         goto retry;
    //     }

    //     if (item->serial != last_item->serial) {
    //         decoder->timer = av_gettime_relative() / 1000000.0;
    //         last_duration = 0.0;
    //     } else {
    //         last_duration = item->pts - last_item->pts;
    //         if (isnan(last_duration) || last_duration <= 0
    //             || last_duration > decoder->max_frame_duration) {
    //             last_duration = last_item->duration;
    //         }
    //     }

    //     delay = 0.0;
    //     time = av_gettime_relative() / 1000000.0;
    //     if (time < decoder->timer + delay) {
    //         goto display;
    //     }

    // display:

    //     return 1;
}
