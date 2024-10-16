#include "maker_pool_c.h"
#include <maker/maker_pool.h>

#include <maker/maker_util.h>

#include "maker_internal.h"

const int _MK_POOL_SLOT_SHIFT = 16;
const int _MK_POOL_SLOT_MASK = ((1 << _MK_POOL_SLOT_SHIFT) - 1);

MKPoolSlotIndex mk_pool_slot_index(MKPoolSlotId slot_id) {
  return (MKPoolSlotIndex){.index = slot_id.id & _MK_POOL_SLOT_MASK};
}

void mk_pool_discard(MKPool *pool) {
  if (pool->gen_ctrs) {
    maker_free(pool->gen_ctrs);
  }
  if (pool->free_slots) {
    maker_free(pool->free_slots);
  }
  pool->free_top = 0;
  pool->size = 0;
  pool->valid = false;
}

bool mk_pool_init(MKPool *pool, uint32_t num_items) {
  MAKER_ASSERT(pool && (num_items > 0) && (num_items < ((1 << 16) - 1)));

  // /* NOTE: item slot 0 is reserved for the special "invalid" item index 0*/
  pool->size = num_items + 1;
  pool->free_top = 0;

  /* generation counters indexable by pool slot index, slot 0 is reserved */
  const size_t gen_ctrs_size = pool->size * sizeof(uint32_t);
  pool->gen_ctrs = (uint32_t *)maker_malloc_clear(gen_ctrs_size);
  MAKER_ASSERT(pool->gen_ctrs);

  /* NOTE: it's not a bug to only reserve num_items here */
  const size_t free_slots_size = num_items * sizeof(uint32_t);
  pool->free_slots = (uint32_t *)maker_malloc_clear(free_slots_size);

  if (pool->free_slots) {
    /* never allocate the 0-th item, this is the reserved 'invalid item' */
    for (uint32_t i = pool->size - 1; i >= 1; i--) {
      pool->free_slots[pool->free_top++] = i;
    }
    pool->valid = true;
  } else {
    mk_pool_discard(pool);
  }

  return pool->valid;
}

MKPoolSlotIndex mk_pool_alloc_item_index(MKPool *pool) {
  MAKER_ASSERT(pool);
  MAKER_ASSERT(pool->free_slots);

  MKPoolSlotIndex slot_index = {.index = 0};

  if (pool->free_top > 0) {
    uint32_t index = pool->free_slots[--pool->free_top];
    MAKER_ASSERT((index >= 0) && (index < pool->size));
    slot_index.index = index;
  }

  return slot_index;
}

MKPoolSlotId
mk_pool_alloc_item(MKPool *pool, MKPoolSlot *slot, MKPoolSlotIndex slot_index) {
  MAKER_ASSERT(pool && pool->valid);
  MAKER_ASSERT(pool->free_slots);
  MAKER_ASSERT((slot_index.index > 0) && (slot_index.index < pool->size));

  uint32_t ctr = ++pool->gen_ctrs[slot_index.index];
  slot->id.id = (ctr << _MK_POOL_SLOT_SHIFT) | (slot_index.index & 0xFFFF);
  slot->state = MK_POOL_ITEM_STATE_ALLOC;

  return slot->id;
}

void mk_pool_free_item(MKPool *pool, MKPoolSlotId slot_id) {
  MAKER_ASSERT(pool && pool->valid);

  MKPoolSlotIndex slot_index = mk_pool_slot_index(slot_id);
  MAKER_ASSERT((slot_index.index >= 0) && (slot_index.index < pool->size));

  pool->free_slots[pool->free_top++] = slot_index.index;
  MAKER_ASSERT(pool->free_top <= (pool->size - 1));
}
