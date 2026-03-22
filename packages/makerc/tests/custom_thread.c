#include "../src/core/maker_internal.h"
#include <maker.h>
#include <stdio.h>
#include <unistd.h>

MakerThreadPool pool = { 0 };

void create_task(MakerStatus (*task)(void* decoder), void* decoder)
{
    printf("Create task\n");
    maker_thread_pool_queue_job(&pool, (MakerStatus (*)(void*))task, decoder);
}

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    maker_thread_pool_init(&pool, 2);

    MakerDecoder decoder = { 0 };
    maker_decoder_init(
        &decoder,
        &(MakerDecoderDesc) {
            .url         = "./tests/video.mp4",
            .use_threads = 1,
            .thread_cb   = create_task,
        }
    );

    MakerMediaInfo info = { 0 };
    maker_decoder_get_media_info(&decoder, &info);

    MakerVideoFrame image = { 0 };
    maker_video_frame_init(
        &image,
        &(MakerVideoFrameDesc) {
            .format = MAKER_PIXEL_FORMAT_RGBA,
            .width  = info.video_width,
            .height = info.video_height,
        }
    );

    printf("sleep 1s...\n");
    sleep(1);
    printf("sleep complete\n");

    maker_decoder_get_video_frame(&decoder, &image);

    printf("Save pgm\n");
    maker_video_frame_save_pgm(&image, "tmp/image.pgm");

    printf("Save ppm\n");
    maker_video_frame_save_ppm(&image, "tmp/image.ppm");

    printf("free image data\n");
    maker_video_frame_uninit(&image);

    printf("free decoder\n");
    maker_decoder_uninit(&decoder);

    maker_media_info_uninit(&info);

    maker_thread_pool_uninit(&pool);

    return 0;
}
