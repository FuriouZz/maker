#include "maker_internal.h"

const i32 _MAKER_POOL_SLOT_SHIFT = 16;
const i32 _MAKER_POOL_SLOT_MASK  = ((1 << _MAKER_POOL_SLOT_SHIFT) - 1);

MakerStatus maker_pool_init(MakerPool* pool, u32 size)
{
    MAKER_ASSERT(pool);
    MAKER_ASSERT(size > 0);

    maker_clear(pool, sizeof(*pool));

    pool->head = 0;
    // Slot 0 is reserved for invalid ID
    pool->size        = size + 1;
    pool->gen_indexes = maker_malloc_clear(sizeof(*pool->gen_indexes) * pool->size);
    pool->slots       = maker_malloc_clear(sizeof(*pool->slots) * size);

    if (pool->gen_indexes == NULL || pool->slots == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    for (u32 i = size; i > 1; i--) {
        pool->slots[pool->head++] = i;
    }

    return MAKER_STATUS_OK;

cleanup:
    maker_free(pool->gen_indexes);
    maker_free(pool->slots);
    return MAKER_STATUS_ERROR;
}

void maker_pool_uninit(MakerPool* pool)
{
    if (pool != NULL) {
        maker_free(pool->gen_indexes);
        maker_free(pool->slots);
    }
}

MakerStatus maker_pool_alloc_slot(MakerPool* pool, MakerPoolSlot* slot)
{
    MAKER_ASSERT(pool);
    MAKER_ASSERT(slot);

    u32 index = 0;
    if (pool->head > 0) {
        index = pool->slots[--pool->head];
        MAKER_ASSERT((index > 0) && (index < pool->size));
    }

    if (index == 0) {
        return MAKER_STATUS_ERROR;
    }

    u32 gen = ++pool->gen_indexes[index];
    u32 id  = (gen << _MAKER_POOL_SLOT_SHIFT) | (index & 0xffff);
    *slot   = id;

    return MAKER_STATUS_OK;
}

u32 maker_pool_get_slot_index(MakerPoolSlot* slot)
{
    MAKER_ASSERT(slot);
    return *slot & _MAKER_POOL_SLOT_MASK;
}

bool maker_pool_is_empty(MakerPool* pool)
{
    MAKER_ASSERT(pool);
    return pool->head == (pool->size - 1);
}
