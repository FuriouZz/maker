#ifndef MK_MEDIA_H
#define MK_MEDIA_H

#include "libavformat/avformat.h"
#include "maker/track.h"

typedef struct MKMedia {
    const char* filename;
    AVFormatContext* format_context;
} MKMedia;

typedef struct MKMediaDesc {
    const char* filename;
} MKMediaDesc;

extern void mk_media_init(MKMedia* media, MKMediaDesc* desc);

extern void mk_media_release(MKMedia* media);

extern unsigned int mk_media_number_of_tracks(MKMedia* media);

extern MKTrackType mk_media_get_track_type(MKMedia* media, unsigned int index);

extern MKTrack mk_media_get_track_with_type(MKMedia* media, MKTrackType type);

#endif
