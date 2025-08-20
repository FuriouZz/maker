#ifndef MK_TRACK_H
#define MK_TRACK_H

#include "libavutil/avutil.h"
#include "maker/maker.h"

extern MKTrackType mk_tracktype_from_avmediatype(enum AVMediaType type);
extern enum AVMediaType mk_tracktype_to_avmediatype(MKTrackType type);

#endif
