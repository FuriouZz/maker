#include "maker_internal.h"

MakerStatus maker_frame_queue_init(MakerFrameQueue* queue, MakerPacketQueue* packet_queue, u32 frame_count)
{
    MAKER_CHECK(queue);
    MAKER_CHECK(packet_queue);

    maker_clear(queue, sizeof(*queue));

    if (maker_mutex_init(&queue->lock) != MAKER_STATUS_OK) {
        goto failed;
    }
    if (maker_cond_init(&queue->new_item_signal) != MAKER_STATUS_OK) {
        goto cleanup_mutex;
    }

    queue->items = maker_malloc_clear(sizeof(AVFrame) * frame_count);
    if (queue->items == NULL) {
        goto cleanup_cond;
    }

    for (u32 i = 0; i < frame_count; i++) {
        AVFrame* frame = av_frame_alloc();
        if (frame == NULL) {
            MAKER_OUT_OF_MEMORY;
            goto cleanup_frames;
        }
        queue->items[i].frame = frame;
    }

    queue->max_frame_count = frame_count;
    queue->packet_queue    = packet_queue;

    return MAKER_STATUS_OK;

cleanup_frames:
    for (u32 i = 0; i < frame_count; i++) {
        av_frame_free(&queue->items[i].frame);
    }

cleanup_cond:
    maker_cond_uninit(&queue->new_item_signal);

cleanup_mutex:
    maker_mutex_uninit(&queue->lock);

failed:
    return MAKER_STATUS_ERROR;
}

void maker_frame_queue_uninit(MakerFrameQueue* queue)
{
    if (queue == NULL) return;

    for (u32 i = 0; i < queue->max_frame_count; i++) {
        av_frame_free(&queue->items[i].frame);
    }

    maker_cond_uninit(&queue->new_item_signal);
    maker_mutex_uninit(&queue->lock);
}

MakerFrameQueueItem* maker_frame_queue_peek_readable(MakerFrameQueue* queue)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return NULL;
    }

    bool* is_aborted = &queue->packet_queue->is_aborted;

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

MakerFrameQueueItem* maker_frame_queue_peek_writable(MakerFrameQueue* queue)
{
    if (queue == NULL) {
        MAKER_LOG_WARN("Invalid value");
        return NULL;
    }

    bool* is_aborted = &queue->packet_queue->is_aborted;

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
    MAKER_ASSERT(queue);

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
    MAKER_ASSERT(queue);
    return &queue->items[queue->read_index];
}

MakerFrameQueueItem* maker_frame_queue_peek_next(MakerFrameQueue* queue)
{
    MAKER_ASSERT(queue);
    u32 index = (queue->read_index + 1) % queue->max_frame_count;
    return &queue->items[index];
}

bool maker_frame_queue_is_full(MakerFrameQueue* queue)
{
    MAKER_ASSERT(queue);

    maker_mutex_lock(&queue->lock);
    bool is_full = queue->frame_count >= queue->max_frame_count;
    maker_mutex_unlock(&queue->lock);
    return is_full;
}
