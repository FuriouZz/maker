#ifndef MAKER_POOL_H
#define MAKER_POOL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct MKPoolSlotId {
  uint32_t id;
} MKPoolSlotId;

typedef struct MKPoolSlotIndex {
  uint32_t index;
} MKPoolSlotIndex;

typedef enum MKPoolItemState {
  MK_POOL_ITEM_STATE_INITIAL,
  MK_POOL_ITEM_STATE_ALLOC,
  MK_POOL_ITEM_STATE_VALID,
  MK_POOL_ITEM_STATE_FAILED,
  MK_POOL_ITEM_STATE_INVALID,
  _MK_POOL_ITEM_STATE_FORCE_U32 = 0x7FFFFFFF
} MKPoolItemState;

typedef struct MKPoolSlot {
  MKPoolSlotId id;
  MKPoolItemState state;
} MKPoolSlot;

typedef struct MKPool {
  uint32_t size;
  uint32_t free_top;
  uint32_t *free_slots;
  uint32_t *gen_ctrs;
  bool valid;
} MKPool;

#endif
