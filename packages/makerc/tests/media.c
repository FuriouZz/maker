#include <maker.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("%s\n", MAKER_VERSION);

    MakerMedia*   media   = maker_media_open("./tests/video.mp4");
    MakerDecoder* decoder = maker_decoder_alloc(
        "./tests/video.mp4",
        (MakerDecoderDesc) { .use_threads = 1 }
    );

    int video_stream_index = media->streams[MAKER_TRACK_TYPE_VIDEO];
    int audio_stream_index = media->streams[MAKER_TRACK_TYPE_AUDIO];

    printf("video=%d audio=%d\n", video_stream_index, audio_stream_index);

    MakerVideoFrame* frame = maker_video_frame_alloc((MakerVideoFrameDesc) {
        .width  = media->video_width,
        .height = media->video_height,
        .format = MAKER_PIXEL_FORMAT_RGBA,
    });

    maker_decoder_start(decoder);

    printf("sleep 1s...\n");
    sleep(1);
    printf("sleep complete\n");

    maker_decoder_get_video_frame(decoder, frame);

    printf("Save pgm\n");
    maker_video_frame_save_pgm(frame, "tmp/image.pgm");

    printf("Save ppm\n");
    maker_video_frame_save_ppm(frame, "tmp/image.ppm");

    printf("stop decoder\n");
    maker_decoder_stop(decoder);

    printf("free image data\n");
    maker_video_frame_free(frame);
    printf("free decoder\n");
    maker_decoder_free(decoder);
    maker_media_free(media);

    return 0;
}
