#ifndef POOL_H
#define POOL_H

#include "maker/maker.h"
#include <stdint.h>

typedef struct MKPoolSlot {
    uint32_t        id;
    MKResourceState state;
} MKPoolSlot;

typedef struct MKPool {
    uint32_t  head;
    uint32_t  size;
    uint32_t* slots;
    uint32_t* gen_indexes;
} MKPool;

extern void     mk_pool_init(MKPool* pool, uint32_t num_items);
extern void     mk_pool_uninit(MKPool* pool);
extern uint32_t mk_pool_alloc_index(MKPool* pool);
extern void     mk_pool_alloc_slot(MKPool* pool, MKPoolSlot* slot, uint32_t index);
extern void     mk_pool_dealloc_slot(MKPool* pool, MKPoolSlot* slot);
extern uint32_t mk_pool_get_index(uint32_t slot_id);
extern uint32_t mk_pool_is_empty(MKPool* pool);

#endif
