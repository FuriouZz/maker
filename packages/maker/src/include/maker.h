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

typedef struct {
    int          streams[MAKER_TRACK_TYPE_COUNT];
    unsigned int video_width;
    unsigned int video_height;
} MakerMedia;

typedef enum {
    MAKER_PIXEL_FORMAT_UNKNOWN = -1,
    MAKER_PIXEL_FORMAT_YUV420P,
    MAKER_PIXEL_FORMAT_RGBA,
    MAKER_PIXEL_FORMAT_RGB,
} MakerPixelFormat;

typedef struct {
    unsigned char* buffer;
    int            buffer_size;

    MakerPixelFormat format;

    unsigned int width;
    unsigned int height;

    unsigned char is_valid; /* boolean */
} MakerImageData;

typedef struct {
    MakerPixelFormat format;

    unsigned int width;
    unsigned int height;
} MakerImageDataDesc;

typedef void MakerDecoder;

typedef struct {
    unsigned char use_threads;
    MakerStatus (*create_thread)(MakerStatus (*task)(MakerDecoder*), MakerDecoder* user_decoder);
} MakerDecoderOptions;

extern MakerMedia* maker_media_open(char* url);
extern void        maker_media_free(MakerMedia* media);

extern MakerImageData* maker_image_data_alloc(MakerImageDataDesc* desc);
extern void            maker_image_data_free(MakerImageData* data);
extern MakerStatus     maker_image_data_init(MakerImageData* data, MakerImageDataDesc* desc);
extern void            maker_image_data_uninit(MakerImageData* data);
extern void            maker_image_data_save_pgm(MakerImageData* target, char* output);
extern void            maker_image_data_save_ppm(MakerImageData* target, char* output);

extern MakerDecoder* maker_decoder_alloc(char* url, MakerDecoderOptions* options);
extern void          maker_decoder_free(MakerDecoder* decoder);
extern MakerStatus   maker_decoder_start(MakerDecoder* decoder);
extern MakerStatus   maker_decoder_stop(MakerDecoder* decoder);
extern unsigned int  maker_decoder_get_frame(MakerDecoder* decoder, MakerImageData* target);
extern MakerStatus   maker_decoder_seek(MakerDecoder* decoder, int seconds);

extern MakerStatus maker_decoder_demux(MakerDecoder* user_decoder);
extern MakerStatus maker_decoder_decode_video(MakerDecoder* user_decoder);

#endif
