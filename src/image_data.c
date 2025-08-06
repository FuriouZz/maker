#include "internal.h"
#include "libavutil/imgutils.h"
#include "maker/format.h"
#include "maker/image_data.h"
#include "maker/utils_mem.h"
#include <stdio.h>

void mk_image_data_init(MKImageData* data, MKImageDataDesc* desc)
{
    MK_ASSERT(!data->is_valid);
    MK_ASSERT(desc);

    enum AVPixelFormat format = mk_format_to_av_pixel_format(desc->format);

    if (format == AV_PIX_FMT_NONE) {
        data->is_valid = 0;
        return;
    }

    int buffer_size
        = av_image_get_buffer_size(format, desc->width, desc->height, 1);

    uint8_t* buffer = mk_malloc(buffer_size * sizeof(uint8_t));
    if (!buffer) {
        data->is_valid = 0;
        return;
    }

    data->buffer = buffer;
    data->buffer_size = buffer_size;
    data->format = desc->format;
    data->width = desc->width;
    data->height = desc->height;
    data->is_valid = 1;
}

MKImageData mk_image_data_make(MKImageDataDesc* desc)
{
    MKImageData data = { 0 };
    mk_image_data_init(&data, desc);
    return data;
}

void mk_image_data_destroy(MKImageData* data)
{
    if (data->is_valid) {
        mk_free(data->buffer);
        data->buffer_size = 0;
        data->width = -1;
        data->height = -1;
        data->format = -1;
        data->is_valid = 0;
    }
}
