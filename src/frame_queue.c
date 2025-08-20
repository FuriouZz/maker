#include "frame_queue.h"
#include "libavutil/error.h"
#include "util.h"

int mk_frame_queue_init(
    MKFrameQueue* queue, MKPacketQueue* packet_queue, int frame_count,
    int keep_last
)
{
    int ret;

    mk_clear(queue, sizeof(MKFrameQueue));
    ret = mk_mutex_init(&queue->mutex);
    if (ret != 0) {
        return ret;
    }

    ret = mk_cond_init(&queue->update_signal);
    if (ret != 0) {
        return ret;
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

void mk_frame_queue_unref_item(MKFrameQueueItem* item)
{
    av_frame_unref(item->frame);
}

void mk_frame_queue_destroy(MKFrameQueue* queue)
{
    int i;
    for (i = 0; i < queue->max_frame_count; i++) {
        MKFrameQueueItem item = queue->items[i];
        mk_frame_queue_unref_item(&item);
        av_frame_free(&item.frame);
    }
    queue->packet_queue = NULL;
    mk_mutex_destroy(&queue->mutex);
    mk_cond_destroy(&queue->update_signal);
}

void mk_frame_queue_trigger_changes(MKFrameQueue* queue)
{
    mk_mutex_lock(&queue->mutex);
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

MKFrameQueueItem* mk_frame_queue_peek(MKFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown)
                 % queue->max_frame_count];
}

MKFrameQueueItem* mk_frame_queue_peek_next(MKFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown + 1)
                 % queue->max_frame_count];
}

MKFrameQueueItem* mk_frame_queue_peek_last(MKFrameQueue* queue)
{
    return &queue->items[queue->read_index];
}

MKFrameQueueItem* mk_frame_queue_peek_readable(MKFrameQueue* queue)
{
    /* wait until we have a readable a new frame */
    mk_mutex_lock(&queue->mutex);
    while (queue->frame_count - queue->is_read_index_shown <= 0
           && queue->packet_queue->is_aborted == 0) {
        mk_cond_wait(&queue->update_signal, &queue->mutex);
    }
    mk_mutex_unlock(&queue->mutex);

    if (queue->packet_queue->is_aborted) {
        return NULL;
    }

    return &queue->items
                [(queue->read_index + queue->is_read_index_shown)
                 % queue->max_frame_count];
}

MKFrameQueueItem* mk_frame_queue_peek_writable(MKFrameQueue* queue)
{
    /* wait until we have space to put a new frame */
    mk_mutex_lock(&queue->mutex);
    while (queue->frame_count >= queue->max_frame_count
           && queue->packet_queue->is_aborted == 0) {
        mk_cond_wait(&queue->update_signal, &queue->mutex);
    }
    mk_mutex_unlock(&queue->mutex);

    if (queue->packet_queue->is_aborted) {
        return NULL;
    }

    return &queue->items[queue->write_index % queue->max_frame_count];
}

void mk_frame_queue_push(MKFrameQueue* queue)
{
    queue->write_index++;
    if (queue->write_index == queue->max_frame_count) {
        queue->write_index = 0;
    }
    mk_mutex_lock(&queue->mutex);
    queue->frame_count++;
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

void mk_frame_queue_next(MKFrameQueue* queue)
{
    if (queue->keep_last_frame && queue->is_read_index_shown == 0) {
        queue->is_read_index_shown = 1;
        return;
    }

    mk_frame_queue_unref_item(&queue->items[queue->read_index]);

    queue->read_index++;
    if (queue->read_index == queue->max_frame_count) {
        queue->read_index = 0;
    }
    mk_mutex_lock(&queue->mutex);
    queue->frame_count--;
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

int mk_frame_queue_remaining_frame_count(MKFrameQueue* queue)
{
    return queue->frame_count - queue->is_read_index_shown;
}

int64_t mk_frame_queue_get_last_shown_position(MKFrameQueue* queue)
{
    MKFrameQueueItem* item = &queue->items[queue->read_index];
    if (queue->is_read_index_shown
        && item->serial == queue->packet_queue->serial) {
        return item->position;
    }
    return -1;
}
