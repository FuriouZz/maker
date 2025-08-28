#include "pool.h"
#include "util.h"
#include <stdio.h>

const int _MK_POOL_SLOT_SHIFT = 16;
const int _MK_POOL_SLOT_MASK  = ((1 << _MK_POOL_SLOT_SHIFT) - 1);

void mk_pool_uninit(MKPool* pool)
{
    MK_ASSERT(pool);

    if (pool->slots != NULL) {
        mk_free(pool->slots);
        pool->slots = NULL;
    }

    if (pool->gen_indexes != NULL) {
        mk_free(pool->gen_indexes);
        pool->gen_indexes = NULL;
    }

    pool->head = 0;
    pool->size = 0;
}

void mk_pool_init(MKPool* pool, uint32_t num_items)
{
    MK_ASSERT(pool);

    pool->slots       = NULL;
    pool->gen_indexes = NULL;
    pool->head        = 0;

    pool->size = num_items + 1;

    pool->gen_indexes = mk_malloc_clear(sizeof(uint32_t) * (num_items + 1));
    MK_ASSERT(pool->gen_indexes);

    pool->slots = mk_malloc_clear(sizeof(uint32_t) * num_items);
    MK_ASSERT(pool->slots);

    for (uint32_t i = num_items; i >= 1; i--) {
        pool->slots[pool->head++] = i;
    }
}

uint32_t mk_pool_alloc_index(MKPool* pool)
{
    MK_ASSERT(pool);

    if (pool->head > 0) {
        uint32_t index = pool->slots[--pool->head];
        MK_ASSERT((index > 0) && (index < pool->size));
        return index;
    }

    return 0;
}

void mk_pool_alloc_slot(MKPool* pool, MKPoolSlot* slot, uint32_t index)
{
    MK_ASSERT(pool);
    MK_ASSERT(index > 0 && index < pool->size);

    uint32_t ctr = ++pool->gen_indexes[index];
    uint32_t id  = (ctr << _MK_POOL_SLOT_SHIFT) | (index & 0xFFFF);
    slot->id     = id;
    slot->state  = MK_RESOURCESTATE_ALLOC;
}

void mk_pool_dealloc_slot(MKPool* pool, MKPoolSlot* slot)
{
    MK_ASSERT(pool);
    MK_ASSERT(slot);

    uint32_t index            = slot->id & _MK_POOL_SLOT_MASK;
    pool->slots[pool->head++] = index;
    MK_ASSERT(pool->head < pool->size);
}

uint32_t mk_pool_get_index(uint32_t slot_id)
{
    return slot_id & _MK_POOL_SLOT_MASK;
}

uint32_t mk_pool_is_empty(MKPool* pool)
{
    return pool->head == (pool->size - 1);
}
