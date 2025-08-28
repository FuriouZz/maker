#include "frame_queue.h"
#include "util.h"

int mk_init_frame_queue(
    MKFrameQueue* queue, MKPacketQueue* packet_queue, int frame_count,
    int keep_last
)
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
    queue->keep_last_frame = !!keep_last;
    queue->packet_queue = packet_queue;

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

_MK_PRIVATE
void mk_unref_frame(MKFrameQueueItem* item) { av_frame_unref(item->frame); }

void mk_uninit_frame_queue(MKFrameQueue* queue)
{
    int i;
    for (i = 0; i < queue->max_frame_count; i++) {
        MKFrameQueueItem item = queue->items[i];
        mk_unref_frame(&item);
        av_frame_free(&item.frame);
    }
    queue->packet_queue = NULL;
    mk_mutex_destroy(&queue->mutex);
    mk_cond_destroy(&queue->update_signal);
}

void mk_trigger_frame_queue_changes(MKFrameQueue* queue)
{
    mk_mutex_lock(&queue->mutex);
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

MKFrameQueueItem* mk_peek_frame(MKFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown)
                 % queue->max_frame_count];
}

MKFrameQueueItem* mk_peek_next_frame(MKFrameQueue* queue)
{
    return &queue->items
                [(queue->read_index + queue->is_read_index_shown + 1)
                 % queue->max_frame_count];
}

MKFrameQueueItem* mk_peek_last_frame(MKFrameQueue* queue)
{
    return &queue->items[queue->read_index];
}

MKFrameQueueItem* mk_peek_readable_frame(MKFrameQueue* queue)
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

MKFrameQueueItem* mk_peek_writable_frame(MKFrameQueue* queue)
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

void mk_push_writable_frame(MKFrameQueue* queue)
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

void mk_drop_frame(MKFrameQueue* queue)
{
    if (queue->keep_last_frame && queue->is_read_index_shown == 0) {
        queue->is_read_index_shown = 1;
        return;
    }

    mk_unref_frame(&queue->items[queue->read_index]);

    queue->read_index++;
    if (queue->read_index == queue->max_frame_count) {
        queue->read_index = 0;
    }
    mk_mutex_lock(&queue->mutex);
    queue->frame_count--;
    mk_cond_signal(&queue->update_signal);
    mk_mutex_unlock(&queue->mutex);
}

int mk_remaining_frame_count(MKFrameQueue* queue)
{
    return queue->frame_count - queue->is_read_index_shown;
}

// int64_t mk_frame_queue_get_last_shown_position(MKFrameQueue* queue)
// {
//     MKFrameQueueItem* item = &queue->items[queue->read_index];
//     if (queue->is_read_index_shown
//         && item->serial == queue->packet_queue->serial) {
//         return item->position;
//     }
//     return -1;
// }
