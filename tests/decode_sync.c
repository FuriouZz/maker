#include "../src/maker_internal.h"
#include <stdint.h>
#include <stdio.h>

#define MK_EXT_IMPL

int main(void)
{
    int     ret;
    MKMedia media = { 0 };

    mk_media_init(&media, &(MKMediaDesc) { .filename = "./tests/video.mp4" });

    MKTrack video_track = { 0 };
    ret                 = mk_media_get_track_from_type(
        &video_track, &media, MK_TRACK_TYPE_VIDEO
    );
    MK_ASSERT(ret == 0);

    MKImageData target = { 0 };
    ret                = mk_image_data_init(
        &target,
        &(MKImageDataDesc) {
                           .width  = video_track.width,
                           .height = video_track.height,
                           .format = MK_PXFMT_RGBA,
        }
    );
    MK_ASSERT(ret == 0);

    // MKMediaDecoder decoder = { 0 };
    // mk_media_decoder_init(&decoder, &media);
    // mk_media_decoder_read_frame(&decoder, &target);

    // mk_image_data_save_pgm(&target, "tmp/image.pgm");
    // mk_image_data_save_ppm(&target, "tmp/image.ppm");

    // mk_media_decoder_free(&decoder);
    return 0;
}
