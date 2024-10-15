#ifndef MAKER_POOL_C_H
#define MAKER_POOL_C_H

#include <maker/maker_pool.h>
#include <stddef.h>

extern MKPoolSlotIndex mk_pool_slot_index(MKPoolSlotId slot_id);

extern void mk_discard_pool(MKPool *pool);

extern bool mk_init_pool(MKPool *pool, uint32_t num_items);

extern MKPoolSlotIndex mk_alloc_pool_item_index(MKPool *pool);

extern MKPoolSlotId
mk_alloc_pool_item(MKPool *pool, MKPoolSlot *slot, MKPoolSlotIndex slot_index);

extern void mk_free_pool_item(MKPool *pool, MKPoolSlotId slot_id);

#endif
