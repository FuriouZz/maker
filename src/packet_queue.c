#include "maker_internal.h"

i32 mk_packet_queue_init(MKPacketQueue* queue)
{
    i32 status;
    status = mk_mutex_init(&queue->mutex);
    if (status != 0) {
        return -1;
    }

    status = mk_cond_init(&queue->new_item_signal);
    if (status != 0) {
        return -1;
    }

    queue->items
        = av_fifo_alloc2(8, sizeof(MKPacketQueueItem), AV_FIFO_FLAG_AUTO_GROW);
    queue->is_aborted = TRUE;
    return 0;
}

void mk_packet_queue_flush(MKPacketQueue* queue)
{
    MKPacketQueueItem pkt;
    mk_mutex_lock(&queue->mutex);
    while (av_fifo_read(queue->items, &pkt, 1) >= 0) {
        av_packet_free(&pkt.packet);
    }
    queue->packet_count = 0;
    queue->duration     = 0;
    queue->byte_size    = 0;
    queue->serial++;
    mk_mutex_unlock(&queue->mutex);
}

void mk_packet_queue_uninit(MKPacketQueue* queue)
{
    mk_packet_queue_flush(queue);
    av_fifo_freep2(&queue->items);
    mk_mutex_destroy(&queue->mutex);
    mk_cond_destroy(&queue->new_item_signal);
}

void mk_packet_queue_start(MKPacketQueue* queue)
{
    mk_mutex_lock(&queue->mutex);
    queue->is_aborted = FALSE;
    queue->serial++;
    mk_mutex_unlock(&queue->mutex);
}

void mk_packet_queue_abort(MKPacketQueue* queue)
{
    mk_mutex_lock(&queue->mutex);
    queue->is_aborted = TRUE;
    mk_cond_signal(&queue->new_item_signal);
    mk_mutex_unlock(&queue->mutex);
}

MK_PRIVATE int
mk_packet_queue_put_private(MKPacketQueue* queue, AVPacket* packet)
{
    MKPacketQueueItem item;
    if (queue->is_aborted) {
        return -1;
    }

    item.packet = packet;
    item.serial = queue->serial;

    i32 ret;

    ret = av_fifo_write(queue->items, &item, 1);
    if (ret < 0) {
        return ret;
    }

    queue->packet_count++;
    queue->duration += packet->duration;
    queue->byte_size += packet->size + sizeof(MKPacketQueueItem);

    mk_cond_signal(&queue->new_item_signal);

    return ret;
}

i32 mk_packet_queue_put(MKPacketQueue* queue, AVPacket* packet)
{
    i32       ret;
    AVPacket* tmp = av_packet_alloc();
    if (!tmp) {
        av_packet_unref(packet);
        return -1;
    }
    av_packet_move_ref(tmp, packet);

    mk_mutex_lock(&queue->mutex);
    ret = mk_packet_queue_put_private(queue, tmp);
    mk_mutex_unlock(&queue->mutex);

    if (ret < 0) {
        av_packet_free(&tmp);
    }

    return ret;
}

i32 mk_packet_queue_get(
    MKPacketQueue* queue, AVPacket* packet, i32 should_block, i32* serial
)
{
    i32               ret;
    MKPacketQueueItem item;

    mk_mutex_lock(&queue->mutex);
    for (;;) {
        if (queue->is_aborted) {
            ret = -1;
            break;
        }

        if (av_fifo_read(queue->items, &item, 1) >= 0) {
            av_packet_move_ref(packet, item.packet);
            if (serial != NULL) {
                *serial = item.serial;
            }
            av_packet_free(&item.packet);

            queue->packet_count--;
            queue->byte_size -= packet->size + sizeof(MKPacketQueueItem);
            queue->duration -= packet->duration;
            ret = 1;
            break;
        } else if (should_block) {
            mk_cond_wait(&queue->new_item_signal, &queue->mutex);
        } else {
            ret = 0;
            break;
        }
    }
    mk_mutex_unlock(&queue->mutex);

    return ret;
}
