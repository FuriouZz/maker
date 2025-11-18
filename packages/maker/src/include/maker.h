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
    MAKER_PIXEL_FORMAT_UNKNOWN = -1,
    MAKER_PIXEL_FORMAT_YUV420P,
    MAKER_PIXEL_FORMAT_RGBA,
    MAKER_PIXEL_FORMAT_RGB,
} MakerPixelFormat;

typedef struct {
    int              streams[MAKER_TRACK_TYPE_COUNT];
    unsigned int     video_width;
    unsigned int     video_height;
    MakerPixelFormat video_format;
} MakerMedia;

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

typedef void MakerDecoder;

typedef struct {
    unsigned char use_threads;
    void (*thread_cb)(MakerStatus (*task)(MakerDecoder* decoder), MakerDecoder* decoder);
} MakerDecoderDesc;

extern MakerMedia* maker_media_open(char* url);
extern void        maker_media_free(MakerMedia* media);

extern MakerVideoFrame* maker_video_frame_alloc(MakerVideoFrameDesc desc);
extern void             maker_video_frame_free(MakerVideoFrame* data);
extern MakerStatus      maker_video_frame_init(MakerVideoFrame* data, MakerVideoFrameDesc desc);
extern void             maker_video_frame_uninit(MakerVideoFrame* data);
extern void             maker_video_frame_save_pgm(MakerVideoFrame* target, char* output);
extern void             maker_video_frame_save_ppm(MakerVideoFrame* target, char* output);

extern MakerDecoder* maker_decoder_alloc(char* url, MakerDecoderDesc desc);
extern void          maker_decoder_free(MakerDecoder* decoder);
extern MakerStatus   maker_decoder_start(MakerDecoder* decoder);
extern MakerStatus   maker_decoder_stop(MakerDecoder* decoder);
extern unsigned int  maker_decoder_get_video_frame(MakerDecoder* decoder, MakerVideoFrame* target);
extern MakerStatus   maker_decoder_seek(MakerDecoder* decoder, int seconds);

extern MakerStatus maker_decoder_demux(MakerDecoder* user_decoder);
extern MakerStatus maker_decoder_decode_video(MakerDecoder* user_decoder);

#endif
