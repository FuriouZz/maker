#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/error.h"
#include "maker/maker.h"
#include "media.h"
#include "track.h"
#include "util.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

int mk_media_init(MKMedia* media, MKMediaDesc* desc)
{
    int ret;
    MK_ASSERT(media);
    MK_ASSERT(desc);

    mk_memset(media->streams, 0, sizeof(media->streams));

    ret = mk_file_exists(desc->filename);
    if (ret != 0) {
        printf("File not found: %s\n", desc->filename);
        return ret;
    }

    AVFormatContext* format = avformat_alloc_context();

    ret = avformat_open_input(&format, desc->filename, NULL, NULL);
    if (ret != 0) {
        printf("Failed to open %s: %s\n", desc->filename, av_err2str(ret));
        return ret;
    }

    ret = avformat_find_stream_info(format, NULL);
    if (ret < 0) {
        printf("Cannot find stream info: %s\n", av_err2str(ret));
        return ret;
    }

    media->streams[MK_TRACK_TYPE_VIDEO]
        = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    media->streams[MK_TRACK_TYPE_AUDIO] = av_find_best_stream(
        format, AVMEDIA_TYPE_AUDIO, -1, media->streams[MK_TRACK_TYPE_VIDEO],
        NULL, 0
    );

    media->filename = desc->filename;
    media->context = mk_malloc_clear(sizeof(MKMediaContext));
    media->context->format = format;

    return 0;
}

void mk_media_destroy(MKMedia* media)
{
    MK_ASSERT(media);
    avformat_free_context(media->context->format);
    mk_free(media->context);
}

unsigned int mk_media_number_of_tracks(MKMedia* media)
{
    MK_ASSERT(media);
    unsigned int count = 0;
    int i;
    for (i = 0; i < MK_TRACK_TYPE_COUNT; i++) {
        if (media->streams[i] > -1) {
            count++;
        }
    }
    return count;
}

MKTrackType mk_media_get_track_type(MKMedia* media, unsigned int index)
{
    MK_ASSERT(media);
    AVStream* stream = media->context->format->streams[index];
    if (!stream) {
        return MK_TRACK_TYPE_UNKNOWN;
    }
    AVCodecParameters* params = stream->codecpar;
    return mk_tracktype_from_avmediatype(params->codec_type);
}

int mk_media_get_track_from_type(
    MKTrack* track, MKMedia* media, MKTrackType type
)
{
    MK_ASSERT(track);
    MK_ASSERT(media);

    int index = media->streams[type];

    if (index == -1) {
        return -1;
    }

    AVStream* stream = media->context->format->streams[index];
    AVCodecParameters* params = stream->codecpar;

    track->stream_index = index;

    if (type == MK_TRACK_TYPE_VIDEO) {
        track->width = params->width;
        track->height = params->height;
        track->format = params->format;
    }

    if (type == MK_TRACK_TYPE_AUDIO) {
        track->format = params->format;
    }

    return 0;
}
