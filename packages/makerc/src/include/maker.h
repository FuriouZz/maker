#ifndef MAKER_H_defined
#define MAKER_H_defined

#define MAKER_VERSION_MAJOR 0
#define MAKER_VERSION_MINOR 0
#define MAKER_VERSION_PATCH 1
#define MAKER_VERSION_EXTRA ""
#define MAKER_VERSION "0.0.1"

/* ---- How to declare API functions ----
 */
#ifndef MAKER_PUBLIC
#ifdef MAKER_WINDOWS
#define MAKER_PUBLIC __declspec(dllexport)
#else
#define MAKER_PUBLIC __attribute__((visibility("default")))
#endif
#endif

/* ---- typedefs ----
 */
typedef enum {
    MAKER_STATUS_ERROR = -1,
    MAKER_STATUS_OK,
    MAKER_STATUS_BUSY,
} MakerStatus;

typedef enum {
    MAKER_TRACK_TYPE_VIDEO,
    MAKER_TRACK_TYPE_AUDIO,
    MAKER_TRACK_TYPE_SUBTITLE,
    MAKER_TRACK_TYPE_COUNT
} MakerTrackType;

typedef enum {
    MAKER_PIXEL_FORMAT_UNKNOWN,
    MAKER_PIXEL_FORMAT_YUV420P,
    MAKER_PIXEL_FORMAT_RGBA,
    MAKER_PIXEL_FORMAT_RGB,
} MakerPixelFormat;

typedef struct {
    int              streams[MAKER_TRACK_TYPE_COUNT];
    unsigned int     video_width;
    unsigned int     video_height;
    MakerPixelFormat video_format;
} MakerMediaInfo;

typedef struct {
    MakerPixelFormat format;
    unsigned int     width;
    unsigned int     height;
} MakerVideoFrameDesc;

typedef struct {
    unsigned char*   buffer;
    int              buffer_size;
    MakerPixelFormat format;
    unsigned int     width;
    unsigned int     height;
    unsigned char    is_valid; /* boolean */
} MakerVideoFrame;

typedef struct {
    void*         internal_state;
    unsigned char is_initialized; /* boolean */
} MakerContext;

typedef struct {
    unsigned int thread_count;
    void (*create_worker)(MakerStatus (*task)(void* decoder), void* decoder);
} MakerContextDesc;

typedef struct {
    void*         internal_state;
    unsigned char is_initialized; /* boolean */
} MakerDecoder;

typedef struct {
    unsigned char     use_playback;
    char*             url;
    MakerContext*     context;
    MakerContextDesc* context_desc;
} MakerDecoderDesc;

MAKER_PUBLIC extern MakerStatus maker_media_info_init(MakerMediaInfo* info, char* url);
MAKER_PUBLIC extern MakerStatus maker_media_info_uninit(MakerMediaInfo* info);

MAKER_PUBLIC extern MakerStatus maker_video_frame_init(MakerVideoFrame* data, MakerVideoFrameDesc* desc);
MAKER_PUBLIC extern MakerStatus maker_video_frame_uninit(MakerVideoFrame* data);
MAKER_PUBLIC extern MakerStatus maker_video_frame_save_pgm(MakerVideoFrame* target, char* output);
MAKER_PUBLIC extern MakerStatus maker_video_frame_save_ppm(MakerVideoFrame* target, char* output);

MAKER_PUBLIC extern MakerStatus  maker_decoder_init(MakerDecoder* decoder, MakerDecoderDesc* desc);
MAKER_PUBLIC extern MakerStatus  maker_decoder_uninit(MakerDecoder* decoder);
MAKER_PUBLIC extern MakerStatus  maker_decoder_get_media_info(MakerDecoder* decoder, MakerMediaInfo* info);
MAKER_PUBLIC extern unsigned int maker_decoder_get_video_frame(MakerDecoder* decoder, MakerVideoFrame* target);
MAKER_PUBLIC extern MakerStatus  maker_decoder_get_playback_time(MakerDecoder* decoder, unsigned int* time_ms);
MAKER_PUBLIC extern MakerStatus  maker_decoder_seek(MakerDecoder* decoder, unsigned long long seconds);

MAKER_PUBLIC extern MakerStatus maker_context_init(MakerContext* context, MakerContextDesc* desc);
MAKER_PUBLIC extern MakerStatus maker_context_uninit(MakerContext* context);

#endif
