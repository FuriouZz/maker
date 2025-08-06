#include "maker/utils_image_data.h"
#include <stdint.h>
#include <stdio.h>

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
