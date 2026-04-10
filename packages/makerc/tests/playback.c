#include <assert.h>
#include <maker.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    MakerDecoder decoder = { 0 };
    maker_decoder_init(
        &decoder,
        &(MakerDecoderDesc) {
            .url          = "./tests/video.mp4",
            .context_desc = &(MakerContextDesc) {
                .thread_count = 2,
            },
        }
    );

    assert(decoder.is_initialized);

    MakerMediaInfo media = { 0 };
    maker_decoder_get_media_info(&decoder, &media);

    MakerVideoFrame frame = { 0 };
    maker_video_frame_init(
        &frame,
        &(MakerVideoFrameDesc) {
            .width  = media.video_width,
            .height = media.video_height,
            .format = MAKER_PIXEL_FORMAT_RGBA,
        }
    );

    MakerClock clock = { 0 };
    maker_clock_init(&clock);
    maker_clock_start(&clock);
    unsigned int time_ms = 0;

    do {
        maker_decoder_get_video_frame(&decoder, &frame);
        maker_clock_get_time(&clock, &time_ms);
    } while (time_ms < 2000);

    maker_clock_uninit(&clock);
    maker_video_frame_uninit(&frame);
    printf("uninit\n");
    maker_decoder_uninit(&decoder);

    return 0;
}
