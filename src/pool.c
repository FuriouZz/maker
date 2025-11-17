#include "maker_internal.h"

const i32 _MK_POOL_SLOT_SHIFT = 16;
const i32 _MK_POOL_SLOT_MASK  = ((1 << _MK_POOL_SLOT_SHIFT) - 1);

void mk_pool_init(MKPool* pool, u32 num_items)
{
    MK_ASSERT(pool);

    pool->slots       = NULL;
    pool->gen_indexes = NULL;
    pool->head        = 0;
    pool->size        = num_items + 1;

    pool->gen_indexes = mk_malloc_clear(sizeof(u32) * (num_items + 1));
    MK_ASSERT(pool->gen_indexes);

    pool->slots = mk_malloc_clear(sizeof(u32) * num_items);
    MK_ASSERT(pool->slots);

    for (u32 i = num_items; i >= 1; i--) {
        pool->slots[pool->head++] = i;
    }
}

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

u32 mk_pool_alloc_index(MKPool* pool)
{
    MK_ASSERT(pool);

    if (pool->head > 0) {
        u32 index = pool->slots[--pool->head];
        MK_ASSERT((index > 0) && (index < pool->size));
        return index;
    }

    return 0;
}

void mk_pool_alloc_slot(MKPool* pool, MKPoolSlot* slot, u32 index)
{
    MK_ASSERT(pool);
    MK_ASSERT(index > 0 && index < pool->size);

    u32 ctr     = ++pool->gen_indexes[index];
    u32 id      = (ctr << _MK_POOL_SLOT_SHIFT) | (index & 0xFFFF);
    slot->id    = id;
    slot->state = MK_RESOURCESTATE_ALLOC;
}

void mk_pool_dealloc_slot(MKPool* pool, MKPoolSlot* slot)
{
    MK_ASSERT(pool);
    MK_ASSERT(slot);

    u32 index                 = slot->id & _MK_POOL_SLOT_MASK;
    pool->slots[pool->head++] = index;
    MK_ASSERT(pool->head < pool->size);
}

u32 mk_pool_get_index(u32 slot_id)
{
    return slot_id & _MK_POOL_SLOT_MASK;
}

u32 mk_pool_is_empty(MKPool* pool)
{
    return pool->head == (pool->size - 1);
}
