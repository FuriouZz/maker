#include "../src/core/maker_internal.h"
#include <maker.h>
#include <stdio.h>
#include <unistd.h>

MakerThreadPool pool = { 0 };

MakerStatus create_task(MakerStatus (*task)(MakerDecoder* decoder), MakerDecoder* decoder)
{
    printf("Create task\n");
    maker_thread_pool_queue_job(&pool, task, decoder);
    return MAKER_STATUS_OK;
}

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    maker_thread_pool_init(&pool, 2);

    MakerMedia*   media   = maker_media_open("./tests/video.mp4");
    MakerDecoder* decoder = maker_decoder_alloc(
        "./tests/video.mp4",
        &(MakerDecoderOptions) {
            .use_threads   = 1,
            .create_thread = create_task,
        }
    );

    int video_stream_index = media->streams[MAKER_TRACK_TYPE_VIDEO];
    int audio_stream_index = media->streams[MAKER_TRACK_TYPE_AUDIO];

    printf("video=%d audio=%d\n", video_stream_index, audio_stream_index);

    MakerImageData* image = maker_image_data_alloc(
        &(MakerImageDataDesc) {
            .format = MAKER_PIXEL_FORMAT_RGBA,
            .width  = media->video_width,
            .height = media->video_height,
        }
    );

    maker_decoder_start(decoder);

    printf("sleep 1s...\n");
    sleep(1);
    printf("sleep complete\n");

    maker_decoder_get_frame(decoder, image);

    printf("Save pgm\n");
    maker_image_data_save_pgm(image, "tmp/image.pgm");

    printf("Save ppm\n");
    maker_image_data_save_ppm(image, "tmp/image.ppm");

    printf("stop decoder\n");
    maker_decoder_stop(decoder);

    printf("free image data\n");
    maker_image_data_free(image);
    printf("free decoder\n");
    maker_decoder_free(decoder);
    maker_media_free(media);

    return 0;
}
