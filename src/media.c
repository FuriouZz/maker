#include "internal.h"
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "maker/log.h"
#include "maker/media.h"
#include "maker/track.h"

void mk_media_init(MKMedia* media, MKMediaDesc* desc)
{
    MK_ASSERT(desc);
    AVFormatContext* format_context = NULL;

    if (avformat_open_input(&format_context, desc->filename, NULL, NULL) != 0) {
        MK_LOG_PANIC(AVFORMAT_OPEN_INPUT_FAILED);
    }

    if (avformat_find_stream_info(format_context, NULL) < 0) {
        MK_LOG_PANIC(AVFORMAT_FIND_STREAM_INFO_FAILED);
    }

    media->filename = desc->filename;
    media->format_context = format_context;
}

void mk_media_release(MKMedia* media)
{
    avformat_free_context(media->format_context);
}

unsigned int mk_media_number_of_tracks(MKMedia* media)
{
    return media->format_context->nb_streams;
}

MKTrackType mk_media_get_track_type(MKMedia* media, unsigned int index)
{
    AVStream* stream = media->format_context->streams[index];
    if (!stream) {
        return MKTRACK_TYPE_UNKNOWN;
    }
    AVCodecParameters* params = stream->codecpar;
    return mk_tracktype_from_avmediatype(params->codec_type);
}

MKTrack mk_media_get_track_with_type(MKMedia* media, MKTrackType type)
{
    MKTrack track = { 0 };
    track.is_valid = 0;

    for (unsigned int i = 0; i < media->format_context->nb_streams; i++) {
        AVStream* stream = media->format_context->streams[i];
        AVCodecParameters* params = stream->codecpar;

        if (mk_tracktype_from_avmediatype(params->codec_type) == type) {
            track.stream_index = i;
            track.is_valid = 1;

            if (type == MKTRACK_TYPE_VIDEO) {
                track.width = params->width;
                track.height = params->height;
                track.format = params->format;
            }

            if (type == MKTRACK_TYPE_AUDIO) {
                track.format = params->format;
            }

            return track;
        }
    }

    return track;
}
