#include "maker_internal.h"

MakerStatus maker_frame_queue_init(MakerFrameQueue* queue, u32 frame_count)
{
    MAKER_CHECK(queue);

    maker_clear(queue, sizeof(*queue));

    if (maker_mutex_init(&queue->lock) != MAKER_STATUS_OK) {
        goto failed;
    }
    if (maker_cond_init(&queue->new_item_signal) != MAKER_STATUS_OK) {
        goto cleanup_mutex;
    }

    for (u32 i = 0; i < 16; i++) {
        AVFrame* frame = av_frame_alloc();
        if (frame == NULL) {
            MAKER_OUT_OF_MEMORY;
            goto cleanup_frames;
        }
        queue->items[i].frame = frame;
    }

    queue->max_frame_count = frame_count;

    return MAKER_STATUS_OK;

cleanup_frames:
    for (u32 i = 0; i < 16; i++) {
        av_frame_free(&queue->items[i].frame);
    }

    // cleanup_cond:
    maker_cond_uninit(&queue->new_item_signal);

cleanup_mutex:
    maker_mutex_uninit(&queue->lock);

failed:
    return MAKER_STATUS_ERROR;
}

void maker_frame_queue_uninit(MakerFrameQueue* queue)
{
    if (queue == NULL) return;

    for (u32 i = 0; i < 16; i++) {
        av_frame_free(&queue->items[i].frame);
    }

    maker_cond_uninit(&queue->new_item_signal);
    maker_mutex_uninit(&queue->lock);
}

MakerFrameQueueItem* maker_frame_queue_peek_readable(MakerFrameQueue* queue, bool* is_aborted)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return NULL;
    }

    maker_mutex_lock(&queue->lock);
    while (queue->frame_count <= 0 && *is_aborted == FALSE) {
        maker_cond_wait(&queue->new_item_signal, &queue->lock);
    }
    maker_mutex_unlock(&queue->lock);

    if (*is_aborted == TRUE) {
        return NULL;
    }

    return &queue->items[queue->read_index % queue->max_frame_count];
}

MakerFrameQueueItem* maker_frame_queue_peek_writable(MakerFrameQueue* queue, bool* is_aborted)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return NULL;
    }

    maker_mutex_lock(&queue->lock);
    while (queue->frame_count >= queue->max_frame_count && *is_aborted == FALSE) {
        maker_cond_wait(&queue->new_item_signal, &queue->lock);
    }
    maker_mutex_unlock(&queue->lock);

    if (*is_aborted == TRUE) {
        return NULL;
    }

    return &queue->items[queue->write_index % queue->max_frame_count];
}

void maker_frame_queue_push_writable(MakerFrameQueue* queue)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return;
    }

    queue->write_index++;
    if (queue->write_index >= queue->max_frame_count) {
        queue->write_index = 0;
    }

    maker_mutex_lock(&queue->lock);
    queue->frame_count++;
    maker_cond_signal(&queue->new_item_signal);
    maker_mutex_unlock(&queue->lock);
}

void maker_frame_queue_pop_readable(MakerFrameQueue* queue)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return;
    }

    av_frame_unref(queue->items[queue->read_index].frame);

    queue->read_index++;
    if (queue->read_index >= queue->max_frame_count) {
        queue->read_index = 0;
    }

    maker_mutex_lock(&queue->lock);
    queue->frame_count--;
    maker_cond_signal(&queue->new_item_signal);
    maker_mutex_unlock(&queue->lock);
}

MakerFrameQueueItem* maker_frame_queue_peek_last(MakerFrameQueue* queue)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return NULL;
    }

    return &queue->items[queue->read_index];
}
