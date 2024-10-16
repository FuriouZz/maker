#ifndef MAKER_POOL_C_H
#define MAKER_POOL_C_H

#include <maker/maker_pool.h>
#include <stddef.h>

extern MKPoolSlotIndex mk_pool_slot_index(MKPoolSlotId slot_id);

extern void mk_pool_discard(MKPool *pool);

extern bool mk_pool_init(MKPool *pool, uint32_t num_items);

extern MKPoolSlotIndex mk_pool_alloc_item_index(MKPool *pool);

extern MKPoolSlotId
mk_pool_alloc_item(MKPool *pool, MKPoolSlot *slot, MKPoolSlotIndex slot_index);

extern void mk_pool_free_item(MKPool *pool, MKPoolSlotId slot_id);

#endif
