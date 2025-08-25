#include "async_decoder.h"
#include "clock.h"
#include "context.h"
#include "format.h"
#include "frame_queue.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/rational.h"
#include "libswscale/swscale.h"
#include "maker/maker.h"
#include "media.h"
#include "util.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int mk_context_create(MKContext* ctx, MKContextDesc* desc)
{
    if (desc == NULL) {
        return -1;
    }

    if (desc->media == NULL) {
        return -1;
    }

    int status;
    MKInternalContext* context = mk_malloc_clear(sizeof(MKInternalContext));
    if (context == NULL) {
        return -1;
    }

    ctx->context = context;
    context->is_aborted = 0;
    context->video_output.pixel_format = MK_PXFMT_RGBA;

    status = mk_clock_init(&context->clock);
    if (status != 0) {
        goto cleanup_context;
    }

    status = mk_async_decoder_init(
        &context->decoder,
        &(MKAsyncDecoderDesc) {
            .media = desc->media,
            .is_aborted = &context->is_aborted,
        }
    );
    if (status != 0) {
        goto cleanup_clock;
    }

    status = mk_async_decoder_start(&context->decoder);
    if (status != 0) {
        goto cleanup_async_decoder;
    }

    return 0;

cleanup_async_decoder:
    mk_async_decoder_destroy(&context->decoder);

cleanup_clock:
    mk_clock_free(&context->clock);

cleanup_context:
    mk_free(ctx->context);
    ctx->context = NULL;

    return -1;
}

int mk_context_start_playback(MKContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;
    mk_clock_start(&context->clock);

    return 0;
}

int mk_context_pause_playback(MKContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;
    mk_clock_pause(&context->clock);

    return 0;
}

int mk_context_get_playback_time(MKContext* ctx, int* time_ms)
{
    if (ctx == NULL) {
        return -1;
    }
    if (time_ms == NULL) {
        return -1;
    }

    int status;
    MKInternalContext* context = ctx->context;
    MKClock* clock = &context->clock;

    MKTime time;
    status = mk_get_time(&time);
    if (status != 0) {
        return -1;
    };

    *time_ms = (time.tv_sec - clock->start_time->tv_sec) * 1000
        + (time.tv_nsec - clock->start_time->tv_nsec) / 1000000;

    return 0;
}

int mk_context_set_playback_time(MKContext* ctx, int time_ms)
{
    if (ctx == NULL) {
        return -1;
    }

    int status;
    MKInternalContext* context = ctx->context;
    MKClock* clock = &context->clock;

    int time_s = time_ms / 1000;
    int time_ns = (time_ms - ((time_ms / 1000) * 1000)) * 1000000;

    MKTime time;
    status = mk_get_time(&time);
    if (status != 0) {
        return -1;
    };

    clock->start_time->tv_sec = time.tv_sec - time_s;
    clock->start_time->tv_nsec = time.tv_nsec - time_ns;

    return 0;
}

int mk_context_destroy(MKContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;
    context->is_aborted = 1;

    mk_async_decoder_stop(&context->decoder);
    mk_async_decoder_destroy(&context->decoder);
    mk_free(context);

    return 0;
}

_MK_PRIVATE int mk_context_video_refresh(MKContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;
    int status;

    MKFrameQueue* picture_queue = &context->decoder.video.frame_q;

    int time_spent_ms;
    status = mk_context_get_playback_time(ctx, &time_spent_ms);
    // printf("timespent=%i\n", time_spent_ms);
    if (status != 0) {
        return -1;
    }

    double time_spent = ((double)time_spent_ms) / 1000.0;
    double time;

    for (;;) {
        if (mk_frame_queue_remaining_frame_count(picture_queue) < 2) {
            // do nothing
        } else {
            MKFrameQueueItem* next = mk_frame_queue_peek_next(picture_queue);

            int stream_index
                = context->decoder.media->streams[MK_TRACK_TYPE_VIDEO];
            AVStream* stream = context->decoder.media->context->format
                                   ->streams[stream_index];

            time = av_q2d(stream->time_base) * next->pts;

            // printf("%f < %f\n", time, time_spent);
            // printf(
            //     // "%f\n", av_q2d(stream->time_base)
            //     "%i/%i\n", stream->time_base.num, stream->time_base.den
            // );

            if (time <= time_spent) {
                mk_frame_queue_next(picture_queue); // drop frame
            } else {
                break;
            }
        }
    }

    return 0;
}

_MK_PRIVATE int
mk_media_async_decoder_yuv2rgb(MKContext* ctx, AVFrame* src_frame)
{
    if (ctx == NULL) {
        return -1;
    }

    int ret;
    MKInternalContext* context = ctx->context;
    MKVideoOutput* output = &context->video_output;
    int width = context->decoder.video.codec_context->width;
    int height = context->decoder.video.codec_context->height;
    int src_format = context->decoder.video.codec_context->pix_fmt;
    enum AVPixelFormat dst_format
        = mk_format_to_av_pixel_format(output->pixel_format);

    if (output->sws_context == NULL) {
        output->sws_context = sws_getContext(
            width, height, src_format, width, height, dst_format, SWS_BILINEAR,
            NULL, NULL, NULL
        );
    }

    if (output->frame == NULL) {
        output->frame = av_frame_alloc();
        ret = av_image_alloc(
            output->frame->data, output->frame->linesize, width, height,
            dst_format, 1
        );
    }

    AVFrame* dst_frame = output->frame;

    ret = sws_scale(
        output->sws_context, (const uint8_t* const*)src_frame->data,
        src_frame->linesize, 0, src_frame->height, dst_frame->data,
        dst_frame->linesize
    );

    if (ret == 0) {
        return -1;
    }

    return 0;
}

int mk_context_get_current_video_frame(MKContext* ctx, MKImageData* target)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;

    int status;
    status = mk_context_video_refresh(ctx);
    if (status != 0) {
        return -1;
    }

    int ret;
    MKFrameQueue* picture_queue = &context->decoder.video.frame_q;
    MKVideoOutput* output = &context->video_output;

    MKFrameQueueItem* item = mk_frame_queue_peek(picture_queue);
    MKFrameQueueItem* next = mk_frame_queue_peek_next(picture_queue);
    if (next->pts == context->next_pts) {
        return item->pts;
    }

    ret = mk_image_data_init(
        target,
        &(MKImageDataDesc) {
            .width = item->width,
            .height = item->height,
            .format = output->pixel_format,
        }
    );

    if (ret != 0) {
        printf("Failed to initialize image data\n");
        return -1;
    }

    if (mk_media_async_decoder_yuv2rgb(ctx, item->frame)) {
        printf("Failed to convert\n");
        return -1;
    }

    ret = av_image_copy_to_buffer(
        (target)->buffer, (target)->buffer_size,
        (const uint8_t* const*)output->frame->data, output->frame->linesize,
        mk_format_to_av_pixel_format(output->pixel_format), (target)->width,
        (target)->height, 1
    );

    if (ret < 0) {
        printf("Failed to copy image data\n");
        return -1;
    }

    context->next_pts = next->pts;
    return item->pts;
}

int mk_context_has_frames(MKContext* ctx)
{
    if (ctx == NULL) {
        return -1;
    }

    MKInternalContext* context = ctx->context;

    return context->decoder.video.is_finished == 0;
}
