#include "maker/log.h"
#include "maker/media.h"
#include "maker/track.h"
#include <stdio.h>

int main(void)
{
    MK_LOG_INFOMSG(OK, "DEMO");

    MKMedia media = { 0 };

    mk_media_init(&media, &(MKMediaDesc) { .filename = "./tests/video.mp4" });

    printf("track count: %u\n", mk_media_number_of_tracks(&media));

    MKTrack video = mk_media_get_track_with_type(&media, MKTRACK_TYPE_VIDEO);
    if (video.is_valid) {
        MKTrackType type = mk_media_get_track_type(&media, video.stream_index);
        if (type == MKTRACK_TYPE_VIDEO) {
            printf("has a video track\n");
        } else {
            printf("unexpected track type\n");
        }
    }

    MKTrack audio = mk_media_get_track_with_type(&media, MKTRACK_TYPE_AUDIO);
    if (audio.is_valid) {
        MKTrackType type = mk_media_get_track_type(&media, audio.stream_index);
        if (type == MKTRACK_TYPE_AUDIO) {
            printf("has a audio track\n");
        } else {
            printf("unexpected track type\n");
        }
    }

    return 0;
}
