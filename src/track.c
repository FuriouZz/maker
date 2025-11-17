#include "maker_internal.h"

MKTrackType mk_tracktype_from_avmediatype(enum AVMediaType type)
{
    switch (type) {
    case AVMEDIA_TYPE_VIDEO: {
        return MK_TRACK_TYPE_VIDEO;
    }
    case AVMEDIA_TYPE_AUDIO: {
        return MK_TRACK_TYPE_AUDIO;
    }
    default: {
        return MK_TRACK_TYPE_UNKNOWN;
    }
    }
}

enum AVMediaType mk_tracktype_to_avmediatype(MKTrackType type)
{
    switch (type) {
    case MK_TRACK_TYPE_VIDEO: {
        return AVMEDIA_TYPE_VIDEO;
    }
    case MK_TRACK_TYPE_AUDIO: {
        return AVMEDIA_TYPE_AUDIO;
    }
    default: {
        return AVMEDIA_TYPE_UNKNOWN;
    }
    }
}
