#include "maker_internal.h"

i32 mk_frame_queue_init(MKFrameQueue* queue, MKPacketQueue* packet_queue, i32 frame_count, boolean keep_last)
{
    int status;

    status = mk_mutex_init(&queue->mutex);
    if (status != 0) {
        return status;
    }

    status = mk_cond_init(&queue->update_signal);
    if (status != 0) {
        return status;
    }
    queue->max_frame_count = FFMIN(frame_count, FRAME_QUEUE_SIZE);
    queue->keep_last_frame = keep_last == TRUE;
    queue->packet_queue    = packet_queue;

    int i;
    for (i = 0; i < queue->max_frame_count; i++) {
        AVFrame* data = av_frame_alloc();
        if (!data) {
            return -1;
        }
        queue->items[i].frame = data;
    }

    return 0;
}

MK_PRIVATE void mk_frame_queue_unref_frame(MKFrameQueueItem* item)
{
    av_frame_unref(item->frame);
}

void mk_frame_queue_uninit(MKFrameQueue* queue)
{
    i32 i;
    for (i = 0; i < queue->max_frame_count; i++) {
        MKFrameQueueItem item = queue->items[i];
        mk_frame_queue_unref_frame(&item);
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
           && queue->packet_queue->is_aborted == FALSE) {
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
           && queue->packet_queue->is_aborted == FALSE) {
        mk_cond_wait(&queue->update_signal, &queue->mutex);
    }
    mk_mutex_unlock(&queue->mutex);

    if (queue->packet_queue->is_aborted) {
        return NULL;
    }

    return &queue->items[queue->write_index % queue->max_frame_count];
}

void mk_frame_queue_push_writable(MKFrameQueue* queue)
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

void mk_frame_queue_drop(MKFrameQueue* queue)
{
    if (queue->keep_last_frame && queue->is_read_index_shown == FALSE) {
        queue->is_read_index_shown = TRUE;
        return;
    }

    mk_frame_queue_unref_frame(&queue->items[queue->read_index]);

    queue->read_index++;
    if (queue->read_index == queue->max_frame_count) {
        queue->read_index = 0;
    }
    mk_mutex_lock(&queue->mutex);
    queue->frame_count--;
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

i32 mk_frame_queue_remaining_frame_count(MKFrameQueue* queue)
{
    return queue->frame_count - queue->is_read_index_shown;
}

// int64 mk_frame_queue_get_last_shown_position(MKFrameQueue* queue)
// {
//     MKFrameQueueItem* item = &queue->items[queue->read_index];
//     if (queue->is_read_index_shown
//         && item->serial == queue->packet_queue->serial) {
//         return item->position;
//     }
//     return -1;
// }
