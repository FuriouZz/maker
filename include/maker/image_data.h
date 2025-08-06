#ifndef MK_IMAGE_DATA_H
#define MK_IMAGE_DATA_H

#include "maker/format.h"
#include <stdint.h>

typedef struct MKImageDataDesc {
    int width;
    int height;
    MKPixelFormat format;
} MKImageDataDesc;

typedef struct MKImageData {
    uint8_t* buffer;
    int buffer_size;
    int width;
    int height;
    MKPixelFormat format;
    int is_valid; /* boolean */
} MKImageData;

extern void mk_image_data_init(MKImageData* data, MKImageDataDesc* desc);
extern void mk_image_data_destroy(MKImageData* data);
extern MKImageData mk_image_data_make(MKImageDataDesc* desc);

#endif
