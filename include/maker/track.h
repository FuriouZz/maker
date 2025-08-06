#ifndef MK_TRACK_H
#define MK_TRACK_H

#include "libavformat/avformat.h"

typedef enum MKTrackType {
    MKTRACK_TYPE_VIDEO,
    MKTRACK_TYPE_AUDIO,
    MKTRACK_TYPE_UNKNOWN
} MKTrackType;

typedef struct MKTrack {
    int width;
    int height;
    int format;
    int stream_index;
    int is_valid; /* boolean */
} MKTrack;

extern MKTrackType mk_tracktype_from_avmediatype(enum AVMediaType type);

#endif
