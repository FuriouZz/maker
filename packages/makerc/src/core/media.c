#include "maker.h"
#include "maker_internal.h"

MakerMedia* maker_media_open(char* url)
{
    AVFormatContext* format = avformat_alloc_context();
    if (format == NULL) {
        MAKER_OUT_OF_MEMORY;
        return NULL;
    }

    if (avformat_open_input(&format, url, NULL, NULL) != 0) {
        MAKER_LOG_WARN("Failed to open input");
        goto cleanup_context;
    }

    if (avformat_find_stream_info(format, NULL) != 0) {
        MAKER_LOG_WARN("Cannot find field info");
        goto cleanup_context;
    }

    MakerMediaInternal* media = maker_malloc_clear(sizeof(*media));
    if (media == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_context;
    }

    media->format = format;
    media->url    = url;
    memset(&media->streams, -1, MAKER_TRACK_TYPE_COUNT);

    media->streams[MAKER_TRACK_TYPE_VIDEO]
        = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    media->streams[MAKER_TRACK_TYPE_AUDIO] = av_find_best_stream(
        format, AVMEDIA_TYPE_AUDIO, -1, media->streams[MAKER_TRACK_TYPE_VIDEO],
        NULL, 0
    );

    media->streams[MAKER_TRACK_TYPE_SUBTITLE] = av_find_best_stream(format, AVMEDIA_TYPE_SUBTITLE, -1, media->streams[MAKER_TRACK_TYPE_AUDIO], NULL, 0);

    if (media->streams[MAKER_TRACK_TYPE_VIDEO] > -1) {
        AVStream* stream    = format->streams[media->streams[MAKER_TRACK_TYPE_VIDEO]];
        media->video_width  = stream->codecpar->width;
        media->video_height = stream->codecpar->height;
        media->video_format = maker_format_from_av_pixel_format(stream->codecpar->format);
    }

    return (MakerMedia*)media;

cleanup_context:
    avformat_free_context(format);

    return NULL;
}

void maker_media_free(MakerMedia* user_media)
{
    if (user_media == NULL) return;

    MakerMediaInternal* media = (MakerMediaInternal*)user_media;

    avformat_free_context(media->format);
    memset(&media->streams, -1, MAKER_TRACK_TYPE_COUNT);
    maker_free(media);
}
