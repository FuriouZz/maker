#include <assert.h>
#include <maker.h>
#include <stdio.h>

int main(void)
{
    printf("==============\n");
    printf("MAKER_VERSION=%s\n", MAKER_VERSION);

    MakerMediaInfo info = { 0 };
    maker_media_info_init(&info, "./tests/video.mp4");

    assert(info.streams[MAKER_TRACK_TYPE_VIDEO] == 0);
    assert(info.streams[MAKER_TRACK_TYPE_AUDIO] == 1);
    assert(info.video_format == MAKER_PIXEL_FORMAT_YUV420P);

    maker_media_info_uninit(&info);

    return 0;
}
