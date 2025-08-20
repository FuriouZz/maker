#include "libavutil/avutil.h"
#include "track.h"

MKTrackType mk_tracktype_from_avmediatype(enum AVMediaType type)
{
    switch (type) {
    case AVMEDIA_TYPE_VIDEO: {
        return MKTRACK_TYPE_VIDEO;
    }
    case AVMEDIA_TYPE_AUDIO: {
        return MKTRACK_TYPE_AUDIO;
    }
    default: {
        return MKTRACK_TYPE_UNKNOWN;
    }
    }
}

enum AVMediaType mk_tracktype_to_avmediatype(MKTrackType type)
{
    switch (type) {
    case MKTRACK_TYPE_VIDEO: {
        return AVMEDIA_TYPE_VIDEO;
    }
    case MKTRACK_TYPE_AUDIO: {
        return AVMEDIA_TYPE_AUDIO;
    }
    default: {
        return AVMEDIA_TYPE_UNKNOWN;
    }
    }
}
