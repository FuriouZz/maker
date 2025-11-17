#include "maker_internal.h"

MakerImageData* maker_image_data_alloc(MakerImageDataDesc* desc)
{
    MakerImageData* data = maker_malloc(sizeof(MakerImageData));
    if (data == NULL) {
        MAKER_OUT_OF_MEMORY;
        return NULL;
    }

    if (desc == NULL) return data;

    if (maker_image_data_init(data, desc) != MAKER_STATUS_OK) {
        maker_free(data);
        return NULL;
    }

    return data;
}

void maker_image_data_free(MakerImageData* data)
{
    if (data == NULL) return;

    maker_image_data_uninit(data);
    maker_free(data);
}

MakerStatus maker_image_data_init(MakerImageData* data, MakerImageDataDesc* desc)
{
    MAKER_CHECK(desc);

    maker_clear(data, sizeof(MakerImageData));

    enum AVPixelFormat format = maker_format_to_av_pixel_format(desc->format);

    if (format == AV_PIX_FMT_NONE) {
        data->is_valid = 0;
        return -1;
    }

    int buffer_size
        = av_image_get_buffer_size(format, desc->width, desc->height, 1);

    u8* buffer = maker_malloc_clear(buffer_size * sizeof(*buffer));
    if (!buffer) {
        data->is_valid = 0;
        return -1;
    }

    data->buffer      = buffer;
    data->buffer_size = buffer_size;
    data->format      = desc->format;
    data->width       = desc->width;
    data->height      = desc->height;
    data->is_valid    = 1;

    return 0;
}

void maker_image_data_uninit(MakerImageData* data)
{
    if (data == NULL) return;

    maker_free(data->buffer);

    data->buffer_size = 0;
    data->width       = -1;
    data->height      = -1;
    data->format      = -1;
    data->is_valid    = FALSE;
}

void maker_image_data_save_pgm(MakerImageData* target, char* output)
{
    FILE* f;
    int   i;
    f = fopen(output, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", target->width, target->height, 255);

    int     item_size = sizeof(uint8_t);
    int     size      = target->width * target->height * item_size;
    uint8_t pixel;

    for (i = 0; i < size; i++) {

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
}

void maker_image_data_save_ppm(MakerImageData* target, char* output)
{
    FILE* f;
    int   i;
    f = fopen(output, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", target->width, target->height, 255);

    int item_size = sizeof(uint8_t);
    int size      = target->width * target->height * item_size;
    for (i = 0; i < size; i++) {
        fwrite(&(target->buffer[i * 4]), item_size, 3, f);
    }
    fclose(f);
}
