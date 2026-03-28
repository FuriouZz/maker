#include "maker_internal.h"

MakerStatus maker_packet_queue_init(MakerPacketQueue* queue)
{
    MAKER_CHECK(queue);

    maker_clear(queue, sizeof(*queue));

    queue->is_aborted = TRUE;

    if (maker_mutex_init(&queue->lock) != MAKER_STATUS_OK) {
        goto failed;
    }

    if (maker_cond_init(&queue->new_item_signal) != MAKER_STATUS_OK) {
        goto cleanup_mutex;
    }

    queue->fifo = av_fifo_alloc2(16, sizeof(MakerPacketQueueItem), AV_FIFO_FLAG_AUTO_GROW);
    if (queue->fifo == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_cond;
    }

    return MAKER_STATUS_OK;

cleanup_cond:
    maker_cond_uninit(&queue->new_item_signal);

cleanup_mutex:
    maker_mutex_uninit(&queue->lock);

failed:
    return MAKER_STATUS_ERROR;
}

void maker_packet_queue_start(MakerPacketQueue* queue)
{
    MAKER_ASSERT(queue);

    maker_mutex_lock(&queue->lock);
    queue->is_aborted = FALSE;
    queue->serial++;
    maker_mutex_unlock(&queue->lock);
}

void maker_packet_queue_stop(MakerPacketQueue* queue)
{
    MAKER_ASSERT(queue);

    maker_mutex_lock(&queue->lock);
    queue->is_aborted = TRUE;
    maker_mutex_unlock(&queue->lock);
}

void maker_packet_queue_flush(MakerPacketQueue* queue)
{
    MAKER_ASSERT(queue);

    MakerPacketQueueItem pkt;
    maker_mutex_lock(&queue->lock);
    while (av_fifo_read(queue->fifo, &pkt, 1) >= 0) {
        av_packet_free(&pkt.packet);
    }
    queue->packet_count = 0;
    queue->serial++;
    maker_mutex_unlock(&queue->lock);
}

void maker_packet_queue_uninit(MakerPacketQueue* queue)
{
    MAKER_ASSERT(queue);

    maker_packet_queue_flush(queue);
    av_fifo_freep2(&queue->fifo);
    maker_mutex_uninit(&queue->lock);
    maker_cond_uninit(&queue->new_item_signal);
}

static MakerStatus
maker__packet_queue_put_private(MakerPacketQueue* queue, AVPacket* packet)
{
    MAKER_ASSERT(queue);
    MAKER_ASSERT(packet);

    MakerPacketQueueItem item;
    if (queue->is_aborted == TRUE) {
        return MAKER_STATUS_ERROR;
    }

    item.packet = packet;
    item.serial = queue->serial;

    if (av_fifo_write(queue->fifo, &item, 1) < 0) {
        return MAKER_STATUS_ERROR;
    }

    queue->packet_count++;

    maker_cond_signal(&queue->new_item_signal);

    return MAKER_STATUS_OK;
}

MakerStatus maker_packet_queue_put(MakerPacketQueue* queue, AVPacket* packet)
{
    MAKER_ASSERT(queue);
    MAKER_ASSERT(packet);

    if (queue->is_aborted) {
        return MAKER_STATUS_ERROR;
    }

    MakerStatus ret;
    AVPacket*   tmp = av_packet_alloc();
    if (tmp == NULL) {
        av_packet_unref(packet);
        MAKER_OUT_OF_MEMORY;
        return MAKER_STATUS_ERROR;
    }
    av_packet_move_ref(tmp, packet);

    maker_mutex_lock(&queue->lock);
    ret = maker__packet_queue_put_private(queue, tmp);
    maker_mutex_unlock(&queue->lock);

    if (ret < 0) {
        av_packet_free(&tmp);
    }

    return ret;
}

i32 maker_packet_queue_get(
    MakerPacketQueue* queue, AVPacket* packet, bool should_block, i32* serial
)
{
    MAKER_ASSERT(queue);
    MAKER_ASSERT(packet);

    i32                  ret;
    MakerPacketQueueItem item;

    maker_mutex_lock(&queue->lock);
    for (;;) {
        if (queue->is_aborted == TRUE) {
            ret = -1;
            break;
        }

        if (av_fifo_read(queue->fifo, &item, 1) >= 0) {
            av_packet_move_ref(packet, item.packet);
            av_packet_free(&item.packet);
            *serial = item.serial;
            queue->packet_count--;
            ret = 1;
            break;
        } else if (should_block == TRUE) {
            maker_cond_wait(&queue->new_item_signal, &queue->lock);
        } else {
            ret = 0;
            break;
        }
    }
    maker_mutex_unlock(&queue->lock);

    return ret;
}
