#include "libavutil/error.h"
#include "maker/ffmpeg_frame_queue.h"
#include "maker/utils_mem.h"

int mk_ffmpeg_frame_queue_init(
    MKFFMpegFrameQueue* queue, MKFFMpegPacketQueue* packet_queue,
    int frame_count, int keep_last
)
{
    mk_clear(queue, sizeof(MKFFMpegFrameQueue));
    queue->mutex = mk_mutex_create();
    if (!queue->mutex) {
        return AVERROR(ENOMEM);
    }
    queue->update_signal = mk_cond_create();
    if (!queue->update_signal) {
        return AVERROR(ENOMEM);
    }
    queue->max_frame_count = FFMIN(frame_count, FRAME_QUEUE_SIZE);
    queue->keep_last_frame = !!keep_last;
    queue->packet_queue = packet_queue;

    int i;
    for (i = 0; i < queue->max_frame_count; i++) {
        AVFrame* data = av_frame_alloc();
        if (!data) {
            return AVERROR(ENOMEM);
        }
        queue->items[i].frame = data;
    }

    return 0;
}

void mk_ffmpeg_frame_queue_unref_item(MKFFMpegFrameQueueItem* item)
{
    av_frame_unref(item->frame);
}

void mk_ffmpeg_frame_queue_destroy(MKFFMpegFrameQueue* queue)
{
    int i;
    for (i = 0; i < queue->max_frame_count; i++) {
        MKFFMpegFrameQueueItem item = queue->items[i];
        mk_ffmpeg_frame_queue_unref_item(&item);
        av_frame_free(&item.frame);
    }
    mk_mutex_destroy(queue->mutex);
    mk_cond_destroy(queue->update_signal);
}

void mk_ffmpeg_frame_queue_trigger_changes(MKFFMpegFrameQueue* queue)
{
    mk_mutex_lock(queue->mutex);
    mk_cond_signal(queue->update_signal);
    mk_mutex_unlock(queue->mutex);
}

MKFFMpegFrameQueueItem* mk_ffmpeg_frame_queue_peek(MKFFMpegFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown)
                 % queue->max_frame_count];
}

MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_next(MKFFMpegFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown + 1)
                 % queue->max_frame_count];
}

MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_last(MKFFMpegFrameQueue* queue)
{
    return &queue->items[queue->read_index];
}

MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_readable(MKFFMpegFrameQueue* queue)
{
    mk_mutex_lock(queue->mutex);
    while (queue->frame_count >= queue->max_frame_count
           && !queue->packet_queue->is_aborted) {
        mk_cond_wait(queue->update_signal, queue->mutex);
    }
    mk_mutex_unlock(queue->mutex);

    if (queue->packet_queue->is_aborted) {
        return NULL;
    }

    return &queue->items
                [(queue->read_index + queue->is_read_index_shown)
                 % queue->max_frame_count];
}

MKFFMpegFrameQueueItem*
mk_ffmpeg_frame_queue_peek_writable(MKFFMpegFrameQueue* queue)
{
    mk_mutex_lock(queue->mutex);
    while (queue->frame_count >= queue->max_frame_count
           && !queue->packet_queue->is_aborted) {
        mk_cond_wait(queue->update_signal, queue->mutex);
    }
    mk_mutex_unlock(queue->mutex);

    if (queue->packet_queue->is_aborted) {
        return NULL;
    }

    return &queue->items[queue->write_index % queue->max_frame_count];
}

void mk_ffmpeg_frame_queue_push(MKFFMpegFrameQueue* queue)
{
    queue->write_index++;
    if (queue->write_index == queue->max_frame_count) {
        queue->write_index = 0;
    }
    mk_mutex_lock(queue->mutex);
    queue->frame_count++;
    mk_cond_signal(queue->update_signal);
    mk_mutex_unlock(queue->mutex);
}

void mk_ffmpeg_frame_queue_next(MKFFMpegFrameQueue* queue)
{
    if (queue->keep_last_frame && !queue->is_read_index_shown) {
        queue->is_read_index_shown = 1;
        return;
    }

    mk_ffmpeg_frame_queue_unref_item(&queue->items[queue->read_index]);

    queue->read_index++;
    if (queue->read_index == queue->max_frame_count) {
        queue->read_index = 0;
    }
    mk_mutex_lock(queue->mutex);
    queue->frame_count--;
    mk_cond_signal(queue->update_signal);
    mk_mutex_unlock(queue->mutex);
}

int mk_ffmpeg_frame_queue_remaining_frame_count(MKFFMpegFrameQueue* queue)
{
    return queue->frame_count - queue->is_read_index_shown;
}

int64_t mk_ffmpeg_frame_queue_get_last_shown_position(MKFFMpegFrameQueue* queue)
{
    MKFFMpegFrameQueueItem* item = &queue->items[queue->read_index];
    if (queue->is_read_index_shown
        && item->serial == queue->packet_queue->serial) {
        return item->position;
    }
    return -1;
}
