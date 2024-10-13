#include "maker_pool_c.h"

#include <maker/maker_util.h>

#include "maker_internal.h"

const int _MK_POOL_SLOT_SHIFT = 16;
const int _MK_POOL_SLOT_MASK = ((1 << _MK_POOL_SLOT_SHIFT) - 1);

uint32_t mk_pool_slot_index(uint32_t slot_id) {
  return slot_id & _MK_POOL_SLOT_MASK;
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
  // /* NOTE: item slot 0 is reserved for the special "invalid" item index 0*/
  pool->size = num_items + 1;
  pool->free_top = 0;

  /* generation counters indexable by pool slot index, slot 0 is reserved */
  const size_t gen_ctrs_size = pool->size * sizeof(uint32_t);
  pool->gen_ctrs = maker_malloc_clear(gen_ctrs_size);

  /* NOTE: it's not a bug to only reserve num_items here */
  const size_t free_slots_size = num_items * sizeof(uint32_t);
  pool->free_slots = maker_malloc_clear(free_slots_size);

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

uint32_t mk_pool_item_alloc_index(MKPool *pool) {
  MAKER_ASSERT(pool);
  MAKER_ASSERT(pool->free_slots);

  if (pool->free_top > 0) {
    uint32_t slot_index = pool->free_slots[--pool->free_top];
    MAKER_ASSERT((slot_index >= 0) && (slot_index < pool->size));
    return slot_index;
  }

  return 0;
}

uint32_t
mk_pool_item_alloc(MKPool *pool, MKPoolSlot *slot, uint32_t slot_index) {
  MAKER_ASSERT(pool && pool->gen_ctrs);
  MAKER_ASSERT((slot_index >= 0) && (slot_index < pool->size));
  MAKER_ASSERT(slot->id == 0);
  MAKER_ASSERT(slot->state == MK_POOL_ITEM_STATE_INITIAL);

  uint32_t ctr = ++pool->gen_ctrs[slot_index];
  slot->id = (ctr << _MK_POOL_SLOT_SHIFT) | (slot_index & 0xFFFF);
  slot->state = MK_POOL_ITEM_STATE_ALLOC;

  return slot->id;
}

void mk_pool_item_free(MKPool *pool, uint32_t slot_id) {
  MAKER_ASSERT(pool && pool->valid);

  uint32_t slot_index = mk_pool_slot_index(slot_id);
  MAKER_ASSERT((slot_index >= 0) && (slot_index < pool->size));

  pool->free_slots[pool->free_top++] = slot_index;
  MAKER_ASSERT(pool->free_top <= (pool->size - 1));
}
