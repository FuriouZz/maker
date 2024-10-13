#ifndef MAKER_POOL_C_H
#define MAKER_POOL_C_H

#include <maker/maker_pool.h>

extern uint32_t mk_pool_slot_index(uint32_t slot_id);

extern void mk_pool_discard(MKPool *pool);

extern bool mk_pool_init(MKPool *pool, uint32_t num_items);

extern uint32_t mk_pool_item_alloc_index(MKPool *pool);

uint32_t extern mk_pool_item_alloc(
    MKPool *pool, MKPoolSlot *slot, uint32_t slot_index
);

extern void mk_pool_item_free(MKPool *pool, uint32_t slot_id);

#endif
