#include "../src/context.h"
#include "../src/util.h"
#include "maker/maker.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int status;
    MKMedia media = { 0 };

    status = mk_media_init(
        &media, &(MKMediaDesc) { .filename = "./tests/video.mp4" }
    );
    MK_ASSERT(status == 0);

    MKTrack video_track = { 0 };
    status = mk_media_get_track_from_type(
        &video_track, &media, MK_TRACK_TYPE_VIDEO
    );
    MK_ASSERT(status == 0);

    MKContext context = { 0 };
    status = mk_context_create(
        &context,
        &(MKContextDesc) {
            .media = &media,
        }
    );
    MK_ASSERT(status == 0);

    status = mk_context_start_playback(&context);
    MK_ASSERT(status == 0);

    MKImageData target = { 0 };

    char buf[128];

    int frame = 0;
    int time_ms;
    while (context.is_eof == 0) {
        mk_context_get_playback_time(&context, &time_ms);
        status = mk_context_get_video_frame(&context, &target);

        if (status > 0 && status != frame) {
            // MK_ASSERT(status == 0);
            frame = status;
            snprintf(buf, 128, "tmp/async2_image_%d.ppm", frame);
            mk_image_data_save_ppm(&target, buf);
        }
    }

    // {
    //     printf("---- 0\n");
    //     status = mk_context_get_video_frame(&context, &target);
    //     MK_ASSERT(status == 0);
    //     mk_image_data_save_ppm(target, "tmp/async2_image0.ppm");
    // }

    // {
    //     printf("---- 1\n");
    //     status = mk_context_get_video_frame(&context, &target);
    //     MK_ASSERT(status == 0);
    //     mk_image_data_save_ppm(target, "tmp/async2_image1.ppm");
    // }

    // {
    //     printf("---- 2\n");
    //     status = mk_context_get_video_frame(&context, &target);
    //     MK_ASSERT(status == 0);
    //     mk_image_data_save_ppm(target, "tmp/async2_image2.ppm");
    // }

    status = mk_context_pause_playback(&context);
    MK_ASSERT(status == 0);

    status = mk_context_destroy(&context);
    MK_ASSERT(status == 0);

    return 0;
}
