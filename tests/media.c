#include "../src/maker_internal.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    MKMedia media = { 0 };
    MK_ASSERT(
        mk_media_init(&media, &(MKMediaDesc) { .filename = "tests/video.mp4" })
        == 0
    );

    MK_ASSERT(mk_media_number_of_tracks(&media) == 2);

    MKTrack video = { 0 };
    MK_ASSERT(
        mk_media_get_track_from_type(&video, &media, MK_TRACK_TYPE_VIDEO) == 0
    );

    MKTrack audio = { 0 };
    MK_ASSERT(
        mk_media_get_track_from_type(&audio, &media, MK_TRACK_TYPE_AUDIO) == 0
    );

    return 0;
}
