#include "../src/media_async_decoder.h"
#include "../src/util.h"
#include "maker/maker.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int ret;
    MKMedia media = { 0 };

    ret = mk_media_init(
        &media, &(MKMediaDesc) { .filename = "./tests/video.mp4" }
    );
    MK_ASSERT(ret == 0);

    MKTrack video_track = { 0 };
    ret = mk_media_get_track_from_type(
        &video_track, &media, MKTRACK_TYPE_VIDEO
    );
    MK_ASSERT(ret == 0);

    MKImageData target = { 0 };
    ret = mk_image_data_init(
        &target,
        &(MKImageDataDesc) {
            .width = video_track.width,
            .height = video_track.height,
            .format = MK_PXFMT_RGBA,
        }
    );
    MK_ASSERT(ret == 0);

    MKMediaAsyncDecoder decoder = { 0 };
    ret = mk_media_async_decoder_init(
        &decoder, &(MKMediaAsyncDecoderDesc) { .media = &media }
    );
    MK_ASSERT(ret == 0);

    ret = mk_media_async_decoder_start(&decoder);
    MK_ASSERT(ret == 0);

    mk_media_async_refresh(&decoder);
    mk_media_async_decoder_get_picture(&decoder, &target);
    mk_image_data_save_ppm(&target, "tmp/async_image0.ppm");

    mk_media_async_refresh(&decoder);
    mk_media_async_decoder_get_picture(&decoder, &target);
    mk_image_data_save_ppm(&target, "tmp/async_image1.ppm");

    mk_media_async_refresh(&decoder);
    mk_media_async_decoder_get_picture(&decoder, &target);
    mk_image_data_save_ppm(&target, "tmp/async_image2.ppm");

    mk_media_async_decoder_stop(&decoder);
    mk_media_async_decoder_destroy(&decoder);

    return 0;
}
