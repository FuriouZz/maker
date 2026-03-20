#include "maker_internal.h"

MakerStatus maker_media_info_init_with_format(MakerMediaInfo* info, AVFormatContext* format)
{
    MAKER_CHECK(info);
    MAKER_CHECK(format);

    maker_memset(&info->streams, -1, MAKER_TRACK_TYPE_COUNT);

    info->streams[MAKER_TRACK_TYPE_VIDEO]
        = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    info->streams[MAKER_TRACK_TYPE_AUDIO] = av_find_best_stream(
        format, AVMEDIA_TYPE_AUDIO, -1, info->streams[MAKER_TRACK_TYPE_VIDEO],
        NULL, 0
    );

    info->streams[MAKER_TRACK_TYPE_SUBTITLE] = av_find_best_stream(format, AVMEDIA_TYPE_SUBTITLE, -1, info->streams[MAKER_TRACK_TYPE_AUDIO], NULL, 0);

    if (info->streams[MAKER_TRACK_TYPE_VIDEO] > -1) {
        AVStream* stream   = format->streams[info->streams[MAKER_TRACK_TYPE_VIDEO]];
        info->video_width  = stream->codecpar->width;
        info->video_height = stream->codecpar->height;
        info->video_format = maker_format_from_av_pixel_format(stream->codecpar->format);
    }

    return MAKER_STATUS_OK;
}

MakerStatus maker_media_info_init(MakerMediaInfo* info, char* url)
{
    MAKER_CHECK(info);
    AVFormatContext* format = maker_media_create_context(url);
    if (format == NULL) {
        return MAKER_STATUS_ERROR;
    }
    return maker_media_info_init_with_format(info, format);
}

MakerStatus maker_media_info_uninit(MakerMediaInfo* info)
{
    MAKER_CHECK(info);

    maker_memset(&info->streams, -1, MAKER_TRACK_TYPE_COUNT);
    info->video_width  = -1;
    info->video_height = -1;
    info->video_format = MAKER_PIXEL_FORMAT_UNKNOWN;

    return MAKER_STATUS_OK;
}
