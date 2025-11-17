#include "maker.h"
#include "maker_internal.h"

MakerStatus maker_packet_queue_init(MakerPacketQueue* queue)
{

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

void maker_packet_queue_flush(MakerPacketQueue* queue)
{
    MakerPacketQueueItem pkt;
    maker_mutex_lock(&queue->lock);
    while (av_fifo_read(queue->fifo, &pkt, 1) >= 0) {
        av_packet_free(&pkt.packet);
    }
    queue->packet_count = 0;
    maker_mutex_unlock(&queue->lock);
}

void maker_packet_queue_uninit(MakerPacketQueue* queue)
{
    maker_packet_queue_flush(queue);
    av_fifo_freep2(&queue->fifo);
    maker_mutex_uninit(&queue->lock);
    maker_cond_uninit(&queue->new_item_signal);
}

static MakerStatus
maker__packet_queue_put_private(MakerPacketQueue* queue, AVPacket* packet, bool* is_aborted)
{
    MakerPacketQueueItem item;
    if (*is_aborted == TRUE) {
        return MAKER_STATUS_ERROR;
    }

    item.packet = packet;

    if (av_fifo_write(queue->fifo, &item, 1) < 0) {
        return MAKER_STATUS_ERROR;
    }

    queue->packet_count++;

    maker_cond_signal(&queue->new_item_signal);

    return MAKER_STATUS_OK;
}

MakerStatus maker_packet_queue_put(MakerPacketQueue* queue, AVPacket* packet, bool* is_aborted)
{
    MakerStatus ret;
    AVPacket*   tmp = av_packet_alloc();
    if (tmp == NULL) {
        av_packet_unref(packet);
        MAKER_OUT_OF_MEMORY;
        return MAKER_STATUS_ERROR;
    }
    av_packet_move_ref(tmp, packet);

    maker_mutex_lock(&queue->lock);
    ret = maker__packet_queue_put_private(queue, tmp, is_aborted);
    maker_mutex_unlock(&queue->lock);

    if (ret < 0) {
        av_packet_free(&tmp);
    }

    return ret;
}

i32 maker_packet_queue_get(
    MakerPacketQueue* queue, AVPacket* packet, bool should_block, bool* is_aborted
)
{
    i32                  ret;
    MakerPacketQueueItem item;

    maker_mutex_lock(&queue->lock);
    for (;;) {
        if (*is_aborted == TRUE) {
            ret = -1;
            break;
        }

        if (av_fifo_read(queue->fifo, &item, 1) >= 0) {
            av_packet_move_ref(packet, item.packet);
            av_packet_free(&item.packet);

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
