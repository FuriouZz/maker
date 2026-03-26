#include <assert.h>
#include <maker.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    MakerContext context = { 0 };
    maker_context_init(
        &context,
        &(MakerContextDesc) {
            .thread_count = 2,
            .use_threads  = 1,
        }
    );

    MakerDecoder decoder = { 0 };
    maker_decoder_init(
        &decoder,
        &context,
        &(MakerDecoderDesc) {
            .url = "./tests/video.mp4",
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

    maker_decoder_get_video_frame(&decoder, &frame, 1);

    printf("Save pgm\n");
    maker_video_frame_save_pgm(&frame, "tmp/image.pgm");

    printf("Save ppm\n");
    maker_video_frame_save_ppm(&frame, "tmp/image.ppm");

    printf("free image data\n");
    maker_video_frame_uninit(&frame);

    printf("free decoder\n");
    maker_decoder_uninit(&decoder);

    printf("free context\n");
    maker_context_uninit(&context);

    return 0;
}
