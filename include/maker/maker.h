#ifndef MAKER_H
#define MAKER_H

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

typedef enum MKTrackType {
    MK_TRACK_TYPE_UNKNOWN = -1,
    MK_TRACK_TYPE_VIDEO,
    MK_TRACK_TYPE_AUDIO,
    MK_TRACK_TYPE_COUNT
} MKTrackType;

typedef enum MKResourceState {
    MK_RESOURCESTATE_INITIAL,
    MK_RESOURCESTATE_ALLOC,
    MK_RESOURCESTATE_VALID,
    MK_RESOURCESTATE_FAILED,
    MK_RESOURCESTATE_INVALID,
    _MK_RESOURCESTATE_FORCE_U32 = 0x7FFFFFFF
} MKResourceState;

typedef struct MKImageDataDesc {
    MKPixelFormat format;
    unsigned int  width;
    unsigned int  height;
} MKImageDataDesc;

typedef struct MKImageData {
    MKPixelFormat  format;
    unsigned int   buffer_size;
    unsigned int   width;
    unsigned int   height;
    unsigned int   is_valid; /* boolean */
    unsigned char* buffer;
} MKImageData;

typedef struct MKMediaHandle {
    unsigned int slot_id;
} MKMediaHandle;

typedef struct MKMediaDesc {
    char* filename;
} MKMediaDesc;

typedef struct MKDecoderHandle {
    unsigned int slot_id;
} MKDecoderHandle;

typedef struct MKContext MKContext;

extern MKContext* mk_context_init(void);
extern void       mk_context_uninit(MKContext* context);

extern MKMediaHandle   mk_context_open_input(MKContext* context, char* filename);
extern MKDecoderHandle mk_context_create_decoder(MKContext* context, MKMediaHandle* media_handle);
extern void            mk_context_drop_decoder(MKContext* context, MKDecoderHandle* handle);

extern void mk_context_start_decoding(MKContext* context);
extern void mk_context_stop_decoding(MKContext* context);

extern int mk_context_get_video_frame(MKContext* context, MKDecoderHandle* handle);
#endif
