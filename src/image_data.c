#include "format.h"
#include "libavutil/imgutils.h"
#include "maker/maker.h"
#include "util.h"
#include <stdio.h>

int mk_image_data_init(MKImageData* data, MKImageDataDesc* desc)
{
    MK_ASSERT(desc);

    mk_clear(data, sizeof(MKImageData));

    enum AVPixelFormat format = mk_format_to_av_pixel_format(desc->format);

    if (format == AV_PIX_FMT_NONE) {
        data->is_valid = 0;
        return -1;
    }

    int buffer_size
        = av_image_get_buffer_size(format, desc->width, desc->height, 1);

    uint8_t* buffer = mk_malloc(buffer_size * sizeof(uint8_t));
    if (!buffer) {
        data->is_valid = 0;
        return -1;
    }

    data->buffer = buffer;
    data->buffer_size = buffer_size;
    data->format = desc->format;
    data->width = desc->width;
    data->height = desc->height;
    data->is_valid = 1;

    return 0;
}

void mk_image_data_destroy(MKImageData* data)
{
    if (data != NULL) {
        mk_free(data->buffer);
        data->buffer_size = 0;
        data->width = -1;
        data->height = -1;
        data->format = -1;
        data->is_valid = 0;
    }
}

void mk_image_data_save_pgm(MKImageData* target, char* output)
{
    FILE* f;
    int i;
    f = fopen(output, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", target->width, target->height, 255);

    int item_size = sizeof(uint8_t);
    int size = target->width * target->height * item_size;
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

void mk_image_data_save_ppm(MKImageData* target, char* output)
{
    FILE* f;
    int i;
    f = fopen(output, "wb");
    fprintf(f, "P6\n%d %d\n%d\n", target->width, target->height, 255);

    int item_size = sizeof(uint8_t);
    int size = target->width * target->height * item_size;
    for (i = 0; i < size; i++) {
        fwrite(&(target->buffer[i * 4]), item_size, 3, f);
    }
    fclose(f);
}
