#include "maker.h"
#include "maker_internal.h"

MakerStatus maker_video_frame_init(MakerVideoFrame* frame, MakerVideoFrameDesc* desc)
{
    MAKER_CHECK(frame);

    maker_clear(frame, sizeof(MakerVideoFrame));

    enum AVPixelFormat format = maker_format_to_av_pixel_format(desc->format);

    if (format == AV_PIX_FMT_NONE) {
        frame->is_valid = FALSE;
        return -1;
    }

    int buffer_size
        = av_image_get_buffer_size(format, desc->width, desc->height, 1);

    u8* buffer = maker_malloc_clear(buffer_size * sizeof(*buffer));
    if (!buffer) {
        frame->is_valid = FALSE;
        return -1;
    }

    frame->buffer      = buffer;
    frame->buffer_size = buffer_size;
    frame->width       = desc->width;
    frame->height      = desc->height;
    frame->format      = desc->format;
    frame->is_valid    = TRUE;

    return 0;
}

MakerStatus maker_video_frame_uninit(MakerVideoFrame* data)
{
    MAKER_CHECK(data);

    maker_free(data->buffer);
    maker_clear(data, sizeof(*data));

    return MAKER_STATUS_OK;
}

MakerStatus maker_video_frame_save_pgm(MakerVideoFrame* target, char* output)
{
    MAKER_CHECK(output);
    MAKER_CHECK(target);
    MAKER_CHECK(target->is_valid);

    FILE* f = fopen(output, "wb");
    MAKER_CHECK(f);

    fprintf(f, "P5\n%d %d\n%d\n", target->width, target->height, 255);

    i32 item_size = sizeof(u8);
    i32 size      = target->width * target->height * item_size;
    u8  pixel;

    for (i32 i = 0; i < size; i++) {

        // https://mmuratarat.github.io/2020-05-13/rgb_to_grayscale_formulas
        // TODO: https://github.com/descampsa/yuv2rgb
        // // Weighted average method
        // pixel = 0.229 * (target->buffer[i * 4])
        //     + 0.587 * (target->buffer[i * 4 + 1])
        //     + 0.114 * (target->buffer[i * 4 + 2]);

        // Luminosity method
        pixel = 0.2126 * (target->buffer[i * 4])
            + 0.7152 * (target->buffer[i * 4 + 1])
            + 0.0722 * (target->buffer[i * 4 + 2]);

        fwrite(&pixel, item_size, 1, f);
    }
    fclose(f);

    return MAKER_STATUS_OK;
}

MakerStatus maker_video_frame_save_ppm(MakerVideoFrame* target, char* output)
{
    MAKER_CHECK(output);
    MAKER_CHECK(target);
    MAKER_CHECK(target->is_valid);

    FILE* f = fopen(output, "wb");
    MAKER_CHECK(f);

    fprintf(f, "P6\n%d %d\n%d\n", target->width, target->height, 255);

    i32 item_size = sizeof(u8);
    i32 size      = target->width * target->height * item_size;
    for (i32 i = 0; i < size; i++) {
        fwrite(&(target->buffer[i * 4]), item_size, 3, f);
    }
    fclose(f);

    return MAKER_STATUS_OK;
}
