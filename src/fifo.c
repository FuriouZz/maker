#include "maker_internal.h"

i32 mk_fifo_init(MKFifo* fifo, usize item_size, usize capacity)
{
    fifo->buffer = mk_malloc_clear(item_size * capacity);
    if (fifo->buffer == NULL) {
        return -1;
    }

    if (mk_mutex_init(&fifo->lock) < 0) {
        mk_free(fifo->buffer);
        return -1;
    }

    fifo->head       = 0;
    fifo->tail       = 0;
    fifo->capacity   = capacity;
    fifo->item_size  = item_size;
    fifo->item_count = 0;

    return 0;
}

void mk_fifo_uninit(MKFifo* fifo)
{
    if (fifo != NULL) {
        mk_mutex_destroy(&fifo->lock);
        mk_free(fifo->buffer);
    }
}

i32 mk_fifo_write(MKFifo* fifo, const void* byte)
{
    MK_CHECK_VALID(fifo);

    mk_mutex_lock(&fifo->lock);

    i32 ret = -1;
    if (fifo->item_count < fifo->capacity) {
        uint8_t* ptr = fifo->buffer + fifo->tail * fifo->item_size;
        memcpy(ptr, byte, fifo->item_size);
        fifo->tail = (fifo->tail + 1) % fifo->capacity;
        fifo->item_count++;
        ret = 0;
    }

    mk_mutex_unlock(&fifo->lock);

    return ret;
}

i32 mk_fifo_can_write(MKFifo* fifo)
{
    MK_CHECK_VALID(fifo);
    return fifo->capacity - fifo->item_count;
}

MK_PRIVATE i32 mk__fifo_read(MKFifo* fifo, void* byte)
{
    MK_CHECK_VALID(fifo);

    i32 ret = -1;
    if (fifo->item_count > 0) {
        uint8_t* ptr = fifo->buffer + fifo->head * fifo->item_size;
        memcpy(byte, ptr, fifo->item_size);
        fifo->head = (fifo->head + 1) % fifo->capacity;
        fifo->item_count--;
        ret = 0;
    }

    return ret;
}

i32 mk_fifo_read(MKFifo* fifo, void* byte)
{
    MK_CHECK_VALID(fifo);

    mk_mutex_lock(&fifo->lock);

    i32 ret = mk__fifo_read(fifo, byte);

    mk_mutex_unlock(&fifo->lock);

    return ret;
}

i32 mk_fifo_can_read(MKFifo* fifo)
{
    MK_CHECK_VALID(fifo);
    return fifo->item_count;
}

i32 mk_fifo_block_read(MKFifo* fifo, void* byte, MKCond* wait, boolean* aborted)
{
    mk_mutex_lock(&fifo->lock);
    int ret = -1;
    while (1) {
        if (aborted != NULL && *aborted == TRUE) {
            ret = -1;
            break;
        }

        if (mk__fifo_read(fifo, byte) == 0) {
            ret = 1;
            break;
        } else if (wait != NULL) {
            mk_cond_wait(wait, &fifo->lock);
        } else {
            ret = 0;
            break;
        }
    }
    mk_mutex_unlock(&fifo->lock);
    return ret;
}
