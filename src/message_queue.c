#include "error.h"
#include "libavutil/fifo.h"
#include "message_queue.h"
#include "mutex.h"
#include "util.h"

void mk_message_queue_init(MKMessageQueue* queue)
{
    if (mk_mutex_init(&queue->mutex) < 0) {
        MK_PANIC("Cannot initialize mutex");
        return;
    }

    queue->fifo
        = av_fifo_alloc2(8, sizeof(MKMessageData), AV_FIFO_FLAG_AUTO_GROW);
    if (queue->fifo == NULL) {
        MK_PANIC("Cannot initialize fifo");
    }
}

void mk_message_queue_uninit(MKMessageQueue* queue)
{
    MK_ASSERT(queue);
    mk_mutex_destroy(&queue->mutex);
    if (queue->fifo) {
        av_fifo_freep2(queue->fifo);
    }
}

int mk_message_queue_send_message(MKMessageQueue* queue, MKMessageData* data)
{
    MK_ASSERT(queue);
    mk_mutex_lock(&queue->mutex);
    int status = av_fifo_write(queue->fifo, data, 1);
    mk_mutex_unlock(&queue->mutex);
    return status;
}

int mk_message_queue_get_message(MKMessageQueue* queue, MKMessageData* data)
{
    MK_ASSERT(queue);

    int status = 0;

    mk_mutex_lock(&queue->mutex);
    if (av_fifo_read(queue->fifo, data, 1) >= 0) {
        status = 1;
    } else {
        status = 0;
    }
    mk_mutex_unlock(&queue->mutex);

    return status;
}

int mk_message_queue_has_messages(MKMessageQueue* queue)
{
    MK_ASSERT(queue);
    return av_fifo_can_read(queue->fifo);
}
