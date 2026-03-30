#ifndef MEDIA_DECODER_INTERNAL_H
#define MEDIA_DECODER_INTERNAL_H

/* ---- dependencies ----
 */
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include <maker.h>

/* ---- defs ---
 */
#ifndef MAKER_ASSERT
#include <assert.h>
#define MAKER_ASSERT(c) assert(c)
#endif

#ifndef TRUE
#define TRUE 0 == 0
#endif
#ifndef FALSE
#define FALSE 0 != 0
#endif

typedef signed char        i8;
typedef unsigned char      u8;
typedef signed short       i16;
typedef unsigned short     u16;
typedef signed int         i32;
typedef unsigned int       u32;
typedef long long          i64;
typedef unsigned long long u64;
typedef float              real32;
typedef double             real64;
typedef unsigned char bool;
typedef size_t usize;

/* ---- util.c ----
 */
extern void* maker_malloc(usize size);
extern void  maker_free(void* ptr);
extern void  maker_clear(void* ptr, usize size);
extern void  maker_memset(void* ptr, usize value, usize size);
extern void* maker_malloc_clear(usize size);
extern void* maker_realloc(void* ptr, usize size);
extern i32   maker_file_exists(char* filename);

/* ---- clock.c ----
 */
typedef struct timespec MakerTime;

typedef struct {
    MakerTime* start_time;
    MakerTime* pause_time;
} MakerClock;

extern MakerStatus maker_clock_init(MakerClock* clock);
extern void        maker_clock_uninit(MakerClock* clock);
extern MakerStatus maker_clock_start(MakerClock* clock);
extern MakerStatus maker_clock_pause(MakerClock* clock);
extern MakerStatus maker_get_time(MakerTime* time);

/* ---- pixel_format.c ----
 */

extern enum AVPixelFormat maker_format_to_av_pixel_format(MakerPixelFormat px_fmt);
extern MakerPixelFormat   maker_format_from_av_pixel_format(enum AVPixelFormat px_fmt);

/* ---- error.c ----
 */
#ifdef MAKER_DEBUG
#define MAKER_FILE __FILE__
#else
#define MAKER_FILE 0
#endif

#define MAKER_LOG_DEBUG(message) maker_log(0, message, __LINE__, MAKER_FILE)
#define MAKER_LOG_INFO(message) maker_log(1, message, __LINE__, MAKER_FILE)
#define MAKER_LOG_WARN(message) maker_log(2, message, __LINE__, MAKER_FILE)
#define MAKER_LOG_ERROR(message) maker_log(3, message, __LINE__, MAKER_FILE)
#define MAKER_PANIC(message) maker_log(4, message, __LINE__, MAKER_FILE)

#define MAKER_OUT_OF_MEMORY MAKER_LOG_ERROR("Out of memory")
#define MAKER_CHECK(v)                       \
    if ((v) == 0) {                          \
        MAKER_LOG_WARN("Check failed: " #v); \
        return MAKER_STATUS_ERROR;           \
    }
#define MAKER_UNUSED(v) (void)(v)

#define MAKER_ATOMIC_COMPARE_EXCHANGE(Ptr, Expected, Desired) \
    __atomic_compare_exchange_n((Ptr), (Expected), (Desired), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define MAKER_ATOMIC_LOAD(Ptr) __atomic_load_n((Ptr), __ATOMIC_SEQ_CST)
#define MAKER_ATOMIC_STORE(Ptr, Value) __atomic_store_n((Ptr), (Value), __ATOMIC_SEQ_CST)
#define MAKER_ATOMIC_ADD(Ptr, Value) __atomic_add_fetch((Ptr), (Value), __ATOMIC_SEQ_CST)

extern void  maker_log(u32 code, char* message, u32 line, char* filename);
extern char* maker_format(char* format, ...);

/* ---- thread.c ----
 */
typedef pthread_mutex_t MakerMutex;
typedef pthread_cond_t  MakerCond;

typedef MakerStatus (*MakerThreadFunction)(void* user_data, u32 user_index);
typedef struct {
    pthread_t           handle;
    MakerThreadFunction callback;
    void*               user_data;
    u32                 user_index;
    MakerStatus         status;
} MakerThread;

typedef struct {
    MakerMutex lock;
    MakerCond  signal;
    i32        index;
    i32        generation_id;
    i32        thread_count;
} MakerBarrier;

extern MakerStatus maker_mutex_init(MakerMutex* mutex);
extern void        maker_mutex_uninit(MakerMutex* mutex);
extern MakerStatus maker_mutex_lock(MakerMutex* mutex);
extern MakerStatus maker_mutex_unlock(MakerMutex* mutex);

extern MakerStatus maker_cond_init(MakerCond* signal);
extern void        maker_cond_uninit(MakerCond* signal);
extern MakerStatus maker_cond_signal(MakerCond* signal);
extern MakerStatus maker_cond_broadcast(MakerCond* signal);
extern MakerStatus maker_cond_wait(MakerCond* signal, MakerMutex* mutex);
extern MakerStatus maker_cond_timedwait(MakerCond* signal, MakerMutex* mutex, i32 seconds);

extern MakerStatus maker_thread_init(MakerThread* thread);
extern MakerStatus maker_thread_wait(MakerThread* thread);

extern MakerStatus maker_barrier_init(MakerBarrier* barrier, i32 thread_count);
extern MakerStatus maker_barrier_uninit(MakerBarrier* barrier);
extern bool        maker_barrier_wait(MakerBarrier* barrier);

/* ---- thread_pool.c ----
 */

typedef struct {
    MakerStatus (*callback)(void* user_data);
    void* data;
} MakerThreadPoolJob;

typedef struct {
    MakerCond  new_job_signal;
    MakerMutex lock;
    AVFifo*    job_queue;
    bool       is_aborted;
} MakerThreadPoolContext;

typedef struct {
    MakerThreadPoolContext context;
    MakerThread*           threads;
    u32                    count;
} MakerThreadPool;

extern MakerThreadPool* maker_thread_pool_alloc(void);
extern void             maker_thread_pool_dealloc(MakerThreadPool* pool);
extern MakerStatus      maker_thread_pool_init(MakerThreadPool* pool, usize thread_count);
extern void             maker_thread_pool_uninit(MakerThreadPool* pool);
extern MakerStatus      maker_thread_pool_queue_job(MakerThreadPool* pool, MakerStatus (*user_job)(void* user_data), void* user_data);
extern i32              maker_thread_pool_job_count(MakerThreadPool* pool);

/* ---- packet_queue.c ----
 */
typedef struct {
    AVPacket* packet;
    u32       serial;
} MakerPacketQueueItem;

typedef struct {
    MakerMutex lock;
    MakerCond  new_item_signal;
    AVFifo*    fifo;
    u32        packet_count;
    i32        serial;
    bool       is_aborted;
} MakerPacketQueue;

extern MakerStatus maker_packet_queue_init(MakerPacketQueue* queue);
extern void        maker_packet_queue_uninit(MakerPacketQueue* queue);
extern void        maker_packet_queue_start(MakerPacketQueue* queue);
extern void        maker_packet_queue_stop(MakerPacketQueue* queue);
extern void        maker_packet_queue_flush(MakerPacketQueue* queue);
extern MakerStatus maker_packet_queue_put(MakerPacketQueue* queue, AVPacket* packet);
extern i32         maker_packet_queue_get(MakerPacketQueue* queue, AVPacket* packet, bool should_block, i32* serial);
extern u32         maker_packet_queue_count(MakerPacketQueue* queue);

/* ---- frame_queue.c ----
 */

typedef struct {
    AVFrame* frame;
} MakerFrameQueueItem;

typedef struct {
    MakerFrameQueueItem* items;
    MakerMutex           lock;
    MakerCond            new_item_signal;
    MakerPacketQueue*    packet_queue;
    u32                  frame_count;
    u32                  max_frame_count;
    u32                  read_index;
    u32                  write_index;
    bool                 is_read_index_shown;
} MakerFrameQueue;

extern MakerStatus          maker_frame_queue_init(MakerFrameQueue* queue, MakerPacketQueue* packet_queue, u32 frame_count);
extern void                 maker_frame_queue_uninit(MakerFrameQueue* queue);
extern MakerFrameQueueItem* maker_frame_queue_peek_readable(MakerFrameQueue* queue);
extern MakerFrameQueueItem* maker_frame_queue_peek_writable(MakerFrameQueue* queue);
extern void                 maker_frame_queue_push_writable(MakerFrameQueue* queue);
extern void                 maker_frame_queue_pop_readable(MakerFrameQueue* queue);
extern MakerFrameQueueItem* maker_frame_queue_peek_last(MakerFrameQueue* queue);
extern bool                 maker_frame_queue_is_full(MakerFrameQueue* queue);

/* ---- media_info.c ----
 */

extern MakerStatus maker_media_info_init_with_format(MakerMediaInfo* info, AVFormatContext* format);

/* ---- media.c ----
 */

typedef struct {
    MakerMediaInfo   info;
    AVFormatContext* format;
    unsigned char    is_initialized; /* boolean */
} MakerMedia;

extern AVFormatContext* maker_media_create_context(char* url);
extern MakerStatus      maker_media_init(MakerMedia* media, char* url);
extern MakerStatus      maker_media_uninit(MakerMedia* media);

/** ---- video_converter.c ----
 */
typedef struct {
    struct SwsContext* sws_context;
    AVFrame*           frame;
} MakerVideoConverter;

extern MakerVideoConverter* maker_video_converter_alloc(void);
extern void                 maker_video_converter_free(MakerVideoConverter* converter);
extern MakerStatus          maker_video_converter_init(MakerVideoConverter* converter, u32 width, u32 height, MakerPixelFormat src_format, MakerPixelFormat dst_format);
extern void                 maker_video_converter_uninit(MakerVideoConverter* converter);
extern MakerStatus          maker_video_converter_yuv2rgba(MakerVideoConverter* converter, MakerVideoFrame* target, AVFrame* src_frame);

/* ---- video_decoder.c ----
 */
typedef struct {
    MakerPacketQueue     packet_queue;
    MakerFrameQueue      frame_queue;
    AVCodecContext*      codec;
    AVPacket*            packet;
    AVFrame*             frame;
    MakerVideoConverter* converter;
    i32                  stream_index;
    i32                  packet_serial;
    bool                 is_aborted;
} MakerVideoDecoder;

typedef struct {
    bool should_wait;
} MakerVideoDecoderOptions;

extern MakerStatus maker_video_decoder_init(MakerVideoDecoder* video, MakerMedia* media);
extern void        maker_video_decoder_uninit(MakerVideoDecoder* video);
extern MakerStatus maker_video_decoder_start(MakerVideoDecoder* video, MakerVideoDecoderOptions* options);
extern MakerStatus maker_video_decoder_stop(MakerVideoDecoder* video);
extern MakerStatus maker_video_decoder_yuv2rgb(MakerVideoDecoder* decoder, MakerVideoFrame* target, AVFrame* src_frame);

/* ---- demuxer.c ----
 */
typedef struct {
    MakerCond          signal;
    AVPacket*          packet;
    MakerVideoDecoder* video;
    AVFormatContext*   format;
    u32                seek_timestamp;
    u32                seek_flags;
    bool               needs_seek;
    bool               is_aborted;
    bool               is_eof;
} MakerDemuxer;

typedef struct {
    u32  max_video_frame_count;
    bool should_wait;
} MakerDemuxerOptions;

extern MakerStatus maker_demuxer_init(MakerDemuxer* demuxer, MakerMedia* media, MakerVideoDecoder* video_decoder);
extern void        maker_demuxer_uninit(MakerDemuxer* demuxer);
extern void        maker_demuxer_setup(MakerDemuxer* demuxer);
extern MakerStatus maker_demuxer_start(MakerDemuxer* demuxer, MakerDemuxerOptions* options);
extern MakerStatus maker_demuxer_stop(MakerDemuxer* demuxer);

/* ---- wait_group.c ----
 */
typedef struct {
    MakerMutex lock;
    MakerCond  signal;
    u32        counter;
} MakerWaitGroup;

extern MakerStatus maker_wait_group_init(MakerWaitGroup* group);
extern MakerStatus maker_wait_group_uninit(MakerWaitGroup* group);
extern void        maker_wait_group_add(MakerWaitGroup* group, u32 value);
extern void        maker_wait_group_wait(MakerWaitGroup* group);
extern void        maker_wait_group_done(MakerWaitGroup* group);

/* ---- decoder.c ----
 */

typedef struct {
    MakerVideoDecoder video;
    MakerDemuxer      demuxer;
    MakerMedia        media;
    MakerClock        clock;
    MakerDecoderDesc  desc;
    bool              use_local_context;
    bool              aborted;
} MakerDecoderInternal;

extern MakerDecoderInternal* maker__decoder_internal(MakerDecoder* user_decoder);

/* ---- context.c ----
 */

typedef struct {
    MakerThreadPool  thread_pool;
    MakerContextDesc desc;
} MakerContextInternal;

extern MakerContextInternal* maker__context_internal(MakerContext* user_context);
#endif
