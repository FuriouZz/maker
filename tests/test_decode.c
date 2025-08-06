#include "maker/utils_image_data.h"
#include <maker/decoder.h>
#include <maker/format.h>
#include <maker/image_data.h>
#include <maker/log.h>
#include <maker/media.h>
#include <maker/track.h>
#include <stdint.h>
#include <stdio.h>

#define MK_EXT_IMPL

#include "mk_ext.h"

int main(void)
{
    MKMedia media = { 0 };

    mk_media_init(&media, &(MKMediaDesc) { .filename = "./tests/video.mp4" });

    MKTrack video_track
        = mk_media_get_track_with_type(&media, MKTRACK_TYPE_VIDEO);
    if (!video_track.is_valid) {
        printf("Invalid video track");
        return -1;
    }

    MKImageData target = mk_image_data_make(&(MKImageDataDesc) {
        .width = video_track.width,
        .height = video_track.height,
        .format = MK_PXFMT_RGBA,
    });

    if (!target.is_valid) {
        printf("Invalid image data");
        return -1;
    }

    MKDecoder decoder = { 0 };
    mk_decoder_start(&decoder, &media);
    mk_decoder_read_frame(&decoder, &target);

    save_pgm(&decoder, "tmp/frame.pgm");
    save_ppm(&decoder, "tmp/frame.ppm");

    mk_decoder_stop(&decoder);

    mk_image_data_save_pgm(&target, "tmp/image.pgm");
    mk_image_data_save_ppm(&target, "tmp/image.ppm");

    return 0;
}
