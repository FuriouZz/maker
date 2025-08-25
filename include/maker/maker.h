#ifndef MAKER_H
#define MAKER_H

#include <stdint.h>

#define MAKER_VERSION_MAJOR 0
#define MAKER_VERSION_MINOR 0
#define MAKER_VERSION_PATCH 1
#define MAKER_VERSION "0.0.1"

typedef enum MKPixelFormat {
    MK_PXFMT_UNKNOWN = -1,
    MK_PXFMT_YUV420P,
    MK_PXFMT_RGBA,
    MK_PXFMT_RGB,
} MKPixelFormat;

typedef struct MKImageDataDesc {
    int width;
    int height;
    MKPixelFormat format;
} MKImageDataDesc;

typedef struct MKImageData {
    uint8_t* buffer;
    int buffer_size;
    int width;
    int height;
    MKPixelFormat format;
    int is_valid; /* boolean */
} MKImageData;

typedef enum MKTrackType {
    MK_TRACK_TYPE_UNKNOWN = -1,
    MK_TRACK_TYPE_VIDEO,
    MK_TRACK_TYPE_AUDIO,
    MK_TRACK_TYPE_COUNT
} MKTrackType;

typedef struct MKTrack {
    int width;
    int height;
    int format;
    int stream_index;
    int is_valid; /* boolean */
} MKTrack;

typedef struct MKMediaContext MKMediaContext;

typedef struct MKMedia {
    const char* filename;
    MKMediaContext* context;
    int streams[MK_TRACK_TYPE_COUNT];
} MKMedia;

typedef struct MKMediaDesc {
    char* filename;
} MKMediaDesc;

typedef struct MKContextDesc {
    MKMedia* media;
} MKContextDesc;

typedef struct MKContext MKContext;

extern int mk_media_init(MKMedia* media, MKMediaDesc* desc);
extern void mk_media_destroy(MKMedia* media);
extern unsigned int mk_media_number_of_tracks(MKMedia* media);
extern MKTrackType mk_media_get_track_type(MKMedia* media, unsigned int index);
extern int
mk_media_get_track_from_type(MKTrack* track, MKMedia* media, MKTrackType type);

extern int mk_image_data_init(MKImageData* data, MKImageDataDesc* desc);
extern void mk_image_data_destroy(MKImageData* data);
extern void mk_image_data_save_pgm(MKImageData* target, char* output);
extern void mk_image_data_save_ppm(MKImageData* target, char* output);

extern int mk_context_create(MKContext* context, MKContextDesc* desc);
extern int mk_context_destroy(MKContext* context);
extern int mk_context_start_playback(MKContext* context);
extern int mk_context_pause_playback(MKContext* context);
extern int mk_context_get_playback_time(MKContext* context, int* time_ms);
extern int
mk_context_get_current_video_frame(MKContext* context, MKImageData* target);
extern int
mk_context_get_next_video_frame(MKContext* context, MKImageData* target);
extern int mk_context_has_frames(MKContext* context);

#endif
