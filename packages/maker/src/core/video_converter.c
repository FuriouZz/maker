#include "maker_internal.h"

MakerVideoConverter* maker_video_converter_alloc(void)
{
    return maker_malloc_clear(sizeof(MakerVideoConverter));
}

void maker_video_converter_free(MakerVideoConverter* converter)
{
    if (converter == NULL) return;
    maker_video_converter_uninit(converter);
    maker_free(converter);
}

MakerStatus maker_video_converter_init(MakerVideoConverter* converter, u32 width, u32 height, MakerPixelFormat user_src_format, MakerPixelFormat user_dst_format)
{
    MAKER_CHECK(converter);

    i32 src_format = maker_format_to_av_pixel_format(user_src_format);
    i32 dst_format = maker_format_to_av_pixel_format(user_dst_format);

    converter->sws_context = sws_getContext(
        width, height, src_format, width, height, dst_format, SWS_BILINEAR,
        NULL, NULL, NULL
    );

    if (converter->sws_context == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto fail;
    }

    converter->frame = av_frame_alloc();
    if (converter->frame == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_sws_context;
    }

    if (av_image_alloc(
            converter->frame->data, converter->frame->linesize, width, height,
            dst_format, 1
        )
        < 0) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_frame;
    }

    converter->frame->format = dst_format;

    return MAKER_STATUS_OK;

cleanup_frame:
    av_frame_free(&converter->frame);

cleanup_sws_context:
    sws_freeContext(converter->sws_context);

fail:
    return MAKER_STATUS_ERROR;
}

void maker_video_converter_uninit(MakerVideoConverter* converter)
{
    if (converter == NULL) return;

    av_frame_free(&converter->frame);
    sws_freeContext(converter->sws_context);
}

MakerStatus maker_video_converter_yuv2rgba(MakerVideoConverter* converter, MakerVideoFrame* target, AVFrame* src_frame)
{
    MAKER_CHECK(converter);
    MAKER_CHECK(target);

    AVFrame* dst_frame = converter->frame;

    if (sws_scale(
            converter->sws_context, (const uint8_t* const*)src_frame->data,
            src_frame->linesize, 0, src_frame->height, dst_frame->data,
            dst_frame->linesize
        )
        == 0) {
        return MAKER_STATUS_ERROR;
    }

    if (av_image_copy_to_buffer(
            (target)->buffer, (target)->buffer_size,
            (const uint8_t* const*)dst_frame->data, dst_frame->linesize,
            dst_frame->format, (target)->width,
            (target)->height, 1
        )
        < 0) {
        MAKER_LOG_ERROR("Failed to copy image data\n");
        return MAKER_STATUS_ERROR;
    }

    return MAKER_STATUS_OK;
}
