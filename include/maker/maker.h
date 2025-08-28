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
    int           width;
    int           height;
    MKPixelFormat format;
} MKImageDataDesc;

typedef struct MKImageData {
    uint8_t*      buffer;
    int           buffer_size;
    int           width;
    int           height;
    MKPixelFormat format;
    int           is_valid; /* boolean */
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
    const char*     filename;
    MKMediaContext* context;
    int             streams[MK_TRACK_TYPE_COUNT];
    int             is_initialized;
} MKMedia;

typedef struct MKMediaDesc {
    char* filename;
} MKMediaDesc;

typedef struct MKContextDesc {
    MKMedia* media;
} MKContextDesc;

typedef struct MKContext {
    void* context;
} MKContext;

extern int         mk_media_init(MKMedia* media, MKMediaDesc* desc);
extern void        mk_media_destroy(MKMedia* media);
extern uint32_t    mk_media_number_of_tracks(MKMedia* media);
extern MKTrackType mk_media_get_track_type(MKMedia* media, unsigned int index);
extern int         mk_media_get_track_from_type(MKTrack* track, MKMedia* media, MKTrackType type);

extern int         mk_image_data_init(MKImageData* data, MKImageDataDesc* desc);
extern void        mk_image_data_destroy(MKImageData* data);
extern void        mk_image_data_save_pgm(MKImageData* target, char* output);
extern void        mk_image_data_save_ppm(MKImageData* target, char* output);

extern int         mk_context_create(MKContext* context, MKContextDesc* desc);
extern int         mk_context_destroy(MKContext* context);
extern int         mk_context_start_playback(MKContext* context);
extern int         mk_context_pause_playback(MKContext* context);
extern int         mk_context_get_playback_time(MKContext* context, int* time_ms);
extern int         mk_context_set_playback_time(MKContext* ctx, int time_ms);
extern int         mk_context_get_current_video_frame(MKContext* context, MKImageData* target);
extern int         mk_context_has_frames(MKContext* context);

// MAKER2

typedef enum MKResourceState {
    MK_RESOURCESTATE_INITIAL,
    MK_RESOURCESTATE_ALLOC,
    MK_RESOURCESTATE_VALID,
    MK_RESOURCESTATE_FAILED,
    MK_RESOURCESTATE_INVALID,
    _MK_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} MKResourceState;

typedef struct MKContext2 MKContext2;

typedef struct MKMediaHandle {
    uint32_t slot_id;
} MKMediaHandle;

typedef struct MKDecoderHandle {
    uint32_t slot_id;
} MKDecoderHandle;

extern char*           mk_get_error(void);
extern MKContext2*     mk_context_init(void);
extern MKMediaHandle   mk_context_open_input(MKContext2* context, char* filename);
extern MKDecoderHandle mk_context_create_decoder(MKContext2* context, MKMediaHandle* media_handle);
extern void            mk_context_drop_decoder(MKContext2* context, MKDecoderHandle* handle);
extern void            mk_context_start_decoding(MKContext2* context);
extern void            mk_context_stop_decoding(MKContext2* context);
extern int             mk_context_get_video_frame(MKContext2* context, MKDecoderHandle* handle);

#endif
