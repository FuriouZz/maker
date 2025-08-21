#include "clock.h"
#include "context.h"
#include "decoder2.h"
#include "format.h"
#include "frame_queue.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "maker/maker.h"
#include "util.h"
#include <time.h>
#include <unistd.h>

int mk_context_create(MKContext* context, MKContextDesc* desc)
{
    if (context == NULL) {
        return -1;
    }

    if (desc->media == NULL) {
        return -1;
    }

    context->is_aborted = 0;
    context->is_eof = 0;
    context->video_output.pixel_format = MK_PXFMT_RGBA;

    int status;
    status = mk_clock_init(&context->clock);
    if (status != 0) {
        return -1;
    }

    status = mk_decoder2_init(
        &context->decoder,
        &(MKDecoder2Desc) {
            .media = desc->media,
            .is_aborted = &context->is_aborted,
            .is_eof = &context->is_eof,
        }
    );
    if (status != 0) {
        mk_clock_free(&context->clock);
        return -1;
    }

    status = mk_decoder2_start(&context->decoder);
    if (status != 0) {
        mk_decoder2_destroy(&context->decoder);
        mk_clock_free(&context->clock);
        return -1;
    }

    return 0;
}

int mk_context_start_playback(MKContext* context)
{
    if (context == NULL) {
        return -1;
    }

    mk_clock_start(&context->clock);

    return 0;
}

int mk_context_pause_playback(MKContext* context)
{
    if (context == NULL) {
        return -1;
    }

    mk_clock_pause(&context->clock);

    return 0;
}

int mk_context_get_playback_time(MKContext* context, int* time_ms)
{
    if (context == NULL) {
        return -1;
    }
    if (time_ms == NULL) {
        return -1;
    }

    int status;
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

int mk_context_destroy(MKContext* context)
{
    if (context == NULL) {
        return -1;
    }

    context->is_aborted = 1;
    mk_decoder2_stop(&context->decoder);
    mk_decoder2_destroy(&context->decoder);

    return 0;
}

_MK_PRIVATE int mk_context_video_refresh(MKContext* context)
{
    if (context == NULL) {
        return -1;
    }

    int status;

    MKFrameQueue* picture_queue = &context->decoder.video.frame_q;

    int time_spent;
    status = mk_context_get_playback_time(context, &time_spent);
    if (status != 0) {
        return -1;
    }

retry:
    if (mk_frame_queue_remaining_frame_count(picture_queue) < 2) {
        goto retry;
    } else {
        MKFrameQueueItem* next = mk_frame_queue_peek_next(picture_queue);

        if (next->pts <= time_spent) {
            mk_frame_queue_next(picture_queue); // drop frame
            goto retry;
        }
    }

    return 0;
}

_MK_PRIVATE int
mk_media_async_decoder_yuv2rgb(MKContext* context, AVFrame* src_frame)
{
    int ret;
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

int mk_context_get_video_frame(MKContext* context, MKImageData* target)
{
    if (context == NULL) {
        return -1;
    }

    int status;
    status = mk_context_video_refresh(context);
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

    if (mk_media_async_decoder_yuv2rgb(context, item->frame)) {
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
