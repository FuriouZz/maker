#ifndef MAKER_INTERNAL_H
#define MAKER_INTERNAL_H

#ifndef MK_PRIVATE
#if defined(__GNUC__) || defined(__clang__)
#define MK_PRIVATE __attribute__((unused)) static
#else
#define MK_PRIVATE static
#endif
#endif

#ifndef MK_ASSERT
#include <assert.h>
#define MK_ASSERT(c) assert(c)
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
typedef unsigned char      boolean;

#include <stdlib.h>
typedef size_t usize;

#ifndef MK_LEN
#define MK_LEN(a) sizeof(a) / sizeof(a[0])
#endif

/* ---- dependencies ----
 */
#include <pthread.h>
#include <time.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/packet.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/fifo.h"

#include "maker/maker.h"

/* ---- error.c ----
 */
#ifdef MK_DEBUG
#define MK_FILE __FILE__
#else
#define MK_FILE 0
#endif

#define MK_LOG_DEBUG(message) mk_log(0, message, __LINE__, MK_FILE)
#define MK_LOG_INFO(message) mk_log(1, message, __LINE__, MK_FILE)
#define MK_LOG_WARN(message) mk_log(2, message, __LINE__, MK_FILE)
#define MK_LOG_ERROR(message) mk_log(3, message, __LINE__, MK_FILE)
#define MK_PANIC(message) mk_log(4, message, __LINE__, MK_FILE)

#define MK_CHECK_VALID(v)             \
    if (!(v)) {                       \
        MK_LOG_WARN("Invalid value"); \
        return -1;                    \
    }

#define MK_CHECK_CLEANUP(v, d) \
    if (!(v)) {                \
        goto d;                \
    }

extern void mk_log(u32 code, char* message, u32 line, char* filename);

/* ---- util.c ----
 */
extern void* mk_malloc(usize size);
extern void  mk_free(void* ptr);
extern void  mk_clear(void* ptr, usize size);
extern void  mk_memset(void* ptr, usize value, usize size);
extern void* mk_malloc_clear(usize size);
extern void* mk_realloc(void* ptr, usize size);
extern i32   mk_file_exists(char* filename);

/* ---- clock.c ----
 */
typedef struct timespec MKTime;

typedef struct MKClock {
    MKTime* start_time;
    MKTime* pause_time;
} MKClock;

extern int  mk_clock_init(MKClock* clock);
extern void mk_clock_free(MKClock* clock);
extern int  mk_clock_start(MKClock* clock);
extern int  mk_clock_pause(MKClock* clock);
extern int  mk_get_time(MKTime* time);

/* ---- mutex.c ----
 */
typedef struct MKMutex {
    pthread_mutex_t handle;
} MKMutex;

typedef struct MKCond {
    pthread_cond_t handle;
} MKCond;

extern i32 mk_mutex_init(MKMutex* mutex);
extern i32 mk_mutex_destroy(MKMutex* mutex);
extern i32 mk_mutex_lock(MKMutex* mutex);
extern i32 mk_mutex_trylock(MKMutex* mutex);
extern i32 mk_mutex_unlock(MKMutex* mutex);
extern i32 mk_cond_init(MKCond* cond);
extern i32 mk_cond_destroy(MKCond* cond);
extern i32 mk_cond_signal(MKCond* cond);
extern i32 mk_cond_broadcast(MKCond* cond);
extern i32 mk_cond_wait(MKCond* cond, MKMutex* mutex);
extern i32 mk_cond_timedwait(MKCond* cond, MKMutex* mutex, i32 seconds);

/* ---- thread.c ----
 */
typedef i32 (*MKThreadFunction)(void* data);

typedef struct MKThread {
    pthread_t        handle;
    const char*      name;
    i32              status;
    MKThreadFunction fn;
    void*            userdata;
} MKThread;

i32 mk_thread_init(MKThread* thread);
i32 mk_thread_wait(MKThread* thread, i32* status);
i32 mk_thread_exit(MKThread* thread);

/* ---- fifo.c ----
 */
typedef struct MKFifo {
    usize    head;
    usize    tail;
    usize    item_size;
    usize    capacity;
    usize    item_count;
    uint8_t* buffer;
    MKMutex  lock;
} MKFifo;

extern i32  mk_fifo_init(MKFifo* fifo, usize item_size, usize count);
extern void mk_fifo_uninit(MKFifo* fifo);
extern i32  mk_fifo_write(MKFifo* fifo, const void* byte);
extern i32  mk_fifo_can_write(MKFifo* fifo);
extern i32  mk_fifo_read(MKFifo* fifo, void* byte);
extern i32  mk_fifo_can_read(MKFifo* fifo);
extern i32  mk_fifo_block_read(MKFifo* fifo, void* byte, MKCond* wait, boolean* aborted);

/* ---- thread_pool.c ----
 */
typedef struct MKThreadPoolContext {
    MKFifo  queue;
    MKCond  signal;
    boolean aborted;
} MKThreadPoolContext;

typedef struct MKThreadJob {
    void* userdata;
    void (*fn)(void* data);
} MKThreadJob;

typedef struct MKThreadPool {
    MKThread*            threads;
    MKThreadPoolContext* context;
    usize                count;
} MKThreadPool;

extern MKThreadPool* mk_thread_pool_alloc(void);
extern void          mk_thread_pool_dealloc(MKThreadPool* pool);
extern i32           mk_thread_pool_init(MKThreadPool* pool, usize thread_count, usize queue_size);
extern i32           mk_thread_pool_uninit(MKThreadPool* pool);
extern i32           mk_thread_pool_queue_job(MKThreadPool* pool, void (*userjob)(void* data), void* userdata);
extern i32           mk_thread_pool_job_count(MKThreadPool* pool);

/* ---- packet_queue.c ----
 */

typedef struct MKPacketQueueItem {
    AVPacket* packet;
    i32       serial; // for video/audio/subtitle sync
} MKPacketQueueItem;

typedef struct MKPacketQueue {
    AVFifo* items;
    MKMutex mutex;
    MKCond  new_item_signal;
    i32     packet_count;
    i32     duration;
    i32     byte_size;
    i32     serial; // for video/audio/subtitle sync
    boolean is_aborted;
} MKPacketQueue;

extern i32  mk_packet_queue_init(MKPacketQueue* queue);
extern void mk_packet_queue_uninit(MKPacketQueue* queue);
extern void mk_packet_queue_start(MKPacketQueue* queue);
extern void mk_packet_queue_abort(MKPacketQueue* queue);
extern void mk_packet_queue_flush(MKPacketQueue* queue);
extern i32  mk_packet_queue_put(MKPacketQueue* queue, AVPacket* packet);
extern i32  mk_packet_queue_get(MKPacketQueue* queue, AVPacket* packet, int should_block, int* serial);

/* ---- frame_queue.c ----
 */
#define FRAME_QUEUE_SIZE 16

typedef struct MKFrameQueueItem {
    // Common
    AVFrame* frame;
    i32      serial; // for video/audio/subtitle sync
    i32      pts; // presentation timestamp for the frame
    i32      duration; // estimated duration of frame
    i64      position; // byte position of the frame in the input file
    i32      format;

    // Video
    i32 width;
    i32 height;
    i32 flip_y; /* boolean */

    // Unsupported
    // AVSubtitle subtitle;
    // AVRational sar;
    // int uploaded;
} MKFrameQueueItem;

typedef struct MKFrameQueue {
    MKPacketQueue*   packet_queue;
    MKFrameQueueItem items[FRAME_QUEUE_SIZE];

    MKMutex mutex;
    MKCond  update_signal;

    i32 read_index;
    i32 write_index;

    i32 frame_count;
    i32 max_frame_count;

    boolean is_read_index_shown;
    boolean keep_last_frame;
} MKFrameQueue;

extern i32               mk_frame_queue_init(MKFrameQueue* queue, MKPacketQueue* packet_queue, i32 frame_count, boolean keep_last);
extern void              mk_frame_queue_uninit(MKFrameQueue* queue);
extern void              mk_frame_queue_trigger_changes(MKFrameQueue* queue);
extern MKFrameQueueItem* mk_frame_queue_peek(MKFrameQueue* queue);
extern MKFrameQueueItem* mk_frame_queue_peek_next(MKFrameQueue* queue);
extern MKFrameQueueItem* mk_frame_queue_peek_last(MKFrameQueue* queue);
extern MKFrameQueueItem* mk_frame_queue_peek_readable(MKFrameQueue* queue);
extern MKFrameQueueItem* mk_frame_queue_peek_writable(MKFrameQueue* queue);
extern void              mk_frame_queue_push_writable(MKFrameQueue* queue);
extern void              mk_frame_queue_drop(MKFrameQueue* queue);
extern i32               mk_frame_queue_remaining_frame_count(MKFrameQueue* queue);
// extern int64 mk_frame_queue_get_last_shown_position(MKFrameQueue* queue);

/* ---- pool.c ----
 */
typedef struct MKPoolSlot {
    u32             id;
    MKResourceState state;
} MKPoolSlot;

typedef struct MKPool {
    u32  head;
    u32  size;
    u32* slots;
    u32* gen_indexes;
} MKPool;

extern void mk_pool_init(MKPool* pool, u32 num_items);
extern void mk_pool_uninit(MKPool* pool);
extern u32  mk_pool_alloc_index(MKPool* pool);
extern void mk_pool_alloc_slot(MKPool* pool, MKPoolSlot* slot, u32 index);
extern void mk_pool_dealloc_slot(MKPool* pool, MKPoolSlot* slot);
extern u32  mk_pool_get_index(u32 slot_id);
extern u32  mk_pool_is_empty(MKPool* pool);

/* ---- media_pool.c ----
 */
#define MK_MAX_MEDIA_POOL_SIZE 20

typedef struct MKMedia2 {
    AVFormatContext* format;
    MKPoolSlot       slot;
    i32              streams[MK_TRACK_TYPE_COUNT];
    char*            filename;
} MKMedia2;

typedef struct MKMediaPool {
    MKMedia2* items;
    MKPool    pool;
} MKMediaPool;

extern void      mk_media_pool_init(MKMediaPool* pool);
extern void      mk_media_pool_uninit(MKMediaPool* pool);
extern void      mk_media_pool_alloc_media(MKMediaPool* pool, MKMediaHandle* handle);
extern void      mk_media_pool_dealloc_media(MKMediaPool* pool, MKMediaHandle* handle);
extern void      mk_media_pool_init_media(MKMediaPool* pool, MKMediaHandle* handle, MKMediaDesc* desc);
extern void      mk_media_pool_uninit_media(MKMediaPool* pool, MKMediaHandle* handle);
extern MKMedia2* mk_media_pool_get_media(MKMediaPool* pool, MKMediaHandle* handle);

/* ---- decoder_pool.c ----
 */

typedef struct MKVideoDecoder {
    AVFormatContext* format;
    AVCodecContext*  codec;

    MKPacketQueue packet_q;
    MKFrameQueue  frame_q;

    AVPacket* packet;

    i32     stream_index;
    boolean is_eof;
} MKVideoDecoder;

typedef struct MKDecoder {
    MKPoolSlot     slot;
    MKVideoDecoder video;
} MKDecoder;

typedef struct MKDecoderPool {
    MKDecoder* items;
    MKPool     pool;
    MKMutex    mutex;
} MKDecoderPool;

extern i32             mk_decoder_pool_init(MKDecoderPool* pool);
extern void            mk_decoder_pool_uninit(MKDecoderPool* pool);
extern MKDecoderHandle mk_decoder_pool_alloc_decoder(MKDecoderPool* pool);
extern void            mk_decoder_pool_dealloc_decoder(MKDecoderPool* pool, MKDecoderHandle* handle);
extern void            mk_decoder_pool_init_decoder(MKDecoderPool* pool, MKDecoderHandle* handle, MKMedia2* media);
extern void            mk_decoder_pool_uninit_decoder(MKDecoderPool* pool, MKDecoderHandle* handle);
extern i32             mk_decoder_has_video_packets(MKDecoderPool* pool, MKDecoderHandle* handle);
extern i32             mk_decoder_has_video_frames(MKDecoderPool* pool, MKDecoderHandle* handle);
extern i32             mk_decoder_pool_demux(MKDecoderPool* pool, MKDecoderHandle* handle, boolean* aborted);
extern i32             mk_decoder_pool_decode_video(MKDecoderPool* pool, MKDecoderHandle* handle, boolean* aborted);

/* ---- track.c ----
 */
extern MKTrackType      mk_tracktype_from_avmediatype(enum AVMediaType type);
extern enum AVMediaType mk_tracktype_to_avmediatype(MKTrackType type);

/* --- format.c ---
 */
extern enum AVPixelFormat mk_format_to_av_pixel_format(MKPixelFormat px_fmt);
extern MKPixelFormat      mk_format_from_av_pixel_format(enum AVPixelFormat px_fmt);

/* ---- maker.c ----
 */
typedef struct MKDecoderData {
    MKDecoderPool*  decoder_p;
    MKDecoderHandle decoder;
    boolean         aborted;
    MKCond*         complete_signal;
} MKDecoderData;

struct MKContext {
    MKThreadPool pool;

    MKMediaPool   media_p;
    MKDecoderPool decoder_p;

    MKDecoderData demux_context;
    MKDecoderData decode_context;
};

#endif
