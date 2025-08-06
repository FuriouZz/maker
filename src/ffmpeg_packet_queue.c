#include "internal.h"
#include "libavutil/error.h"
#include "maker/ffmpeg_packet_queue.h"

int mk_ffmpeg_packet_queue_init(MKFFMpegPacketQueue* queue)
{
    queue->mutex = mk_mutex_create();
    if (!queue->mutex) {
        return AVERROR(ENOMEM);
    }
    queue->new_item_signal = mk_cond_create();
    if (!queue->new_item_signal) {
        return AVERROR(ENOMEM);
    }
    queue->items = av_fifo_alloc2(8, sizeof(AVPacket), AV_FIFO_FLAG_AUTO_GROW);
    queue->is_aborted = 1;
    return 0;
}

void mk_ffmpeg_packet_queue_flush(MKFFMpegPacketQueue* queue)
{
    MKFFMpegPacketQueueItem pkt;
    mk_mutex_lock(queue->mutex);
    while (av_fifo_read(queue->items, &pkt, 1) >= 0) {
        av_packet_free(&pkt.packet);
    }
    queue->packet_count = 0;
    queue->duration = 0;
    queue->byte_size = 0;
    queue->serial++;
    mk_mutex_unlock(queue->mutex);
}

void mk_ffmpeg_packet_queue_free(MKFFMpegPacketQueue* queue)
{
    mk_ffmpeg_packet_queue_flush(queue);
    av_fifo_freep2(&queue->items);
    mk_mutex_destroy(queue->mutex);
    mk_cond_destroy(queue->new_item_signal);
}

void mk_ffmpeg_packet_queue_start(MKFFMpegPacketQueue* queue)
{
    mk_mutex_lock(queue->mutex);
    queue->is_aborted = 0;
    queue->serial++;
    mk_mutex_unlock(queue->mutex);
}

void mk_ffmpeg_packet_queue_abort(MKFFMpegPacketQueue* queue)
{
    mk_mutex_lock(queue->mutex);
    queue->is_aborted = 0;
    mk_cond_signal(queue->new_item_signal);
    mk_mutex_unlock(queue->mutex);
}
_MK_PRIVATE int
mk_packet_queue_put_private(MKFFMpegPacketQueue* queue, AVPacket* packet)
{
    MKFFMpegPacketQueueItem item;
    if (queue->is_aborted) {
        return -1;
    }

    item.packet = packet;
    item.serial = queue->serial;

    int ret;

    ret = av_fifo_write(queue->items, &item, 1);
    if (ret < 0) {
        return ret;
    }

    queue->packet_count++;
    queue->duration += packet->duration;
    queue->byte_size += packet->size + sizeof(MKFFMpegPacketQueueItem);

    mk_cond_signal(queue->new_item_signal);

    return ret;
}

int mk_ffmpeg_packet_queue_put(MKFFMpegPacketQueue* queue, AVPacket* packet)
{
    int ret;
    AVPacket* tmp = av_packet_alloc();
    if (!tmp) {
        av_packet_unref(packet);
        return -1;
    }

    av_packet_move_ref(tmp, packet);

    mk_mutex_lock(queue->mutex);
    ret = mk_packet_queue_put_private(queue, tmp);
    mk_mutex_unlock(queue->mutex);

    if (ret < 0) {
        av_packet_free(&tmp);
    }

    return ret;
}

int mk_ffmpeg_packet_queue_get(
    MKFFMpegPacketQueue* queue, AVPacket* packet, int should_block, int* serial
)
{
    int ret;
    MKFFMpegPacketQueueItem item;

    mk_mutex_lock(queue->mutex);
    for (;;) {
        if (queue->is_aborted) {
            ret = -1;
            break;
        }

        if (av_fifo_read(queue->items, &item, 1) >= 0) {

            av_packet_move_ref(packet, item.packet);
            if (serial) {
                *serial = item.serial;
            }
            av_packet_free(&item.packet);

            queue->packet_count--;
            queue->byte_size -= packet->size + sizeof(MKFFMpegPacketQueueItem);
            queue->duration -= packet->duration;
            ret = 1;
            break;
        } else if (should_block) {
            mk_cond_wait(queue->new_item_signal, queue->mutex);
        } else {
            ret = 0;
            break;
        }
    }
    mk_mutex_unlock(queue->mutex);

    return ret;
}
