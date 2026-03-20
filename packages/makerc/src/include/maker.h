#ifndef MAKER_H_defined
#define MAKER_H_defined

#define MAKER_VERSION_MAJOR 0
#define MAKER_VERSION_MINOR 0
#define MAKER_VERSION_PATCH 1
#define MAKER_VERSION_EXTRA ""
#define MAKER_VERSION "0.0.1"

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
} MakerDecoder;

typedef struct {
    unsigned char use_playback;
    unsigned char use_threads;
    char*         url;
    void (*thread_cb)(MakerStatus (*task)(MakerDecoder* decoder), MakerDecoder* decoder);
} MakerDecoderDesc;

extern MakerStatus maker_media_info_init(MakerMediaInfo* media, char* url);
extern MakerStatus maker_media_info_uninit(MakerMediaInfo* media);

extern MakerStatus maker_video_frame_init(MakerVideoFrame* data, MakerVideoFrameDesc* desc);
extern void        maker_video_frame_uninit(MakerVideoFrame* data);
extern MakerStatus maker_video_frame_save_pgm(MakerVideoFrame* target, char* output);
extern MakerStatus maker_video_frame_save_ppm(MakerVideoFrame* target, char* output);

extern MakerStatus maker_decoder_init(MakerDecoder* decoder, MakerDecoderDesc* desc);
extern MakerStatus maker_decoder_uninit(MakerDecoder* decoder);
extern MakerStatus maker_decoder_get_media_info(MakerDecoder* user_decoder, MakerMediaInfo* media);

extern unsigned int maker_decoder_get_video_frame(MakerDecoder* decoder, MakerVideoFrame* target);
extern MakerStatus  maker_decoder_get_playback_time(MakerDecoder* decoder, unsigned int* time_ms);
extern MakerStatus  maker_decoder_seek(MakerDecoder* decoder, unsigned long long seconds);

extern MakerStatus maker_decoder_demux(MakerDecoder* user_decoder);
extern MakerStatus maker_decoder_decode_video(MakerDecoder* user_decoder);

#endif
