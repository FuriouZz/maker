#include "maker/track.h"

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
    return MKTRACK_TYPE_UNKNOWN;
}
