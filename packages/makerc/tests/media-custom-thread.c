#include "../src/core/maker_internal.h"
#include <maker.h>
#include <stdio.h>
#include <unistd.h>

MakerThreadPool pool = { 0 };

void create_task(MakerStatus (*task)(MakerDecoder* decoder), MakerDecoder* decoder)
{
    printf("Create task\n");
    maker_thread_pool_queue_job(&pool, task, decoder);
}

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    maker_thread_pool_init(&pool, 2);

    MakerMedia*   media   = maker_media_open("./tests/video.mp4");
    MakerDecoder* decoder = maker_decoder_alloc(
        "./tests/video.mp4",
        (MakerDecoderDesc) {
            .use_threads = 1,
            .thread_cb   = create_task,
        }
    );

    int video_stream_index = media->streams[MAKER_TRACK_TYPE_VIDEO];
    int audio_stream_index = media->streams[MAKER_TRACK_TYPE_AUDIO];

    printf("video=%d audio=%d\n", video_stream_index, audio_stream_index);

    MakerVideoFrame* image = maker_video_frame_alloc(
        (MakerVideoFrameDesc) {
            .format = MAKER_PIXEL_FORMAT_RGBA,
            .width  = media->video_width,
            .height = media->video_height,
        }
    );

    maker_decoder_start(decoder);

    printf("sleep 1s...\n");
    sleep(1);
    printf("sleep complete\n");

    maker_decoder_get_video_frame(decoder, image);

    printf("Save pgm\n");
    maker_video_frame_save_pgm(image, "tmp/image.pgm");

    printf("Save ppm\n");
    maker_video_frame_save_ppm(image, "tmp/image.ppm");

    printf("stop decoder\n");
    maker_decoder_stop(decoder);

    printf("free image data\n");
    maker_video_frame_free(image);
    printf("free decoder\n");
    maker_decoder_free(decoder);
    maker_media_free(media);

    maker_thread_pool_uninit(&pool);

    return 0;
}
