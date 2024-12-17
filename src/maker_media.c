#include <maker/maker_media.h>

#include <maker/maker_util.h>

#include <stdio.h>
#include <string.h>

#include "maker/maker_pool.h"
#include "maker_internal.h"
#include "maker_pool_c.h"

_MAKER_PRIVATE MKMediaPoolItem *
_mk_media_lookup(MKMediaPool *pool, MKPoolSlotId slot_id) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  MKPoolSlotIndex slot_index = mk_pool_slot_index(slot_id);
  MAKER_ASSERT((slot_index.index > 0) && (slot_index.index < pool->pool.size));

  MKMediaPoolItem *items = pool->items;
  MKMediaPoolItem *item = &items[slot_index.index];

  if (item->slot.id.id == slot_id.id) {
    return item;
  }

  return NULL;
}

bool mk_media_pool_init(MKMediaPool *pool, size_t item_count) {
  // MAKER_ASSERT(0 == pool->items);
  if (!mk_pool_init(&pool->pool, item_count)) {
    return false;
  }

  pool->items = maker_malloc_clear(pool->pool.size * sizeof(MKMediaPoolItem));
  if (!pool->items) {
    mk_media_pool_free(pool);
  }

  return pool->pool.valid;
}

void mk_media_pool_free(MKMediaPool *pool) {
  mk_pool_discard(&pool->pool);
  if (pool->items) {
    maker_free(pool->items);
  }
}

MKMediaHandle mk_media_alloc(MKMediaPool *pool) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  MKPoolSlotIndex slot_index = mk_pool_alloc_item_index(&pool->pool);
  MKMediaHandle handle = {.id = 0};

  if (slot_index.index != 0) {
    MKMediaPoolItem *item = &pool->items[slot_index.index];
    handle.id = mk_pool_alloc_item(&pool->pool, &item->slot, slot_index);
  }

  return handle;
}

void mk_media_init(
    MKMediaPool *pool, MKMediaHandle handle, const char *filename
) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  MKPoolSlotIndex slot_index = mk_pool_slot_index(handle.id);

  MAKER_ASSERT((slot_index.index > 0) && (slot_index.index < pool->pool.size));

  MKMediaPoolItem *item = _mk_media_lookup(pool, handle.id);
  if (item) {
    if (item->slot.state == MK_POOL_ITEM_STATE_ALLOC) {
      item->media = (MKMedia){.filename = filename};
      item->slot.state = MK_POOL_ITEM_STATE_VALID;
    }
  }
}

MKMediaHandle mk_media_create(MKMediaPool *pool, const char *filename) {
  MKMediaHandle handle = mk_media_alloc(pool);
  mk_media_init(pool, handle, filename);
  return handle;
}

void mk_media_uninit(MKMediaPool *pool, MKMediaHandle *handle) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  MKPoolSlotIndex slot_index = mk_pool_slot_index(handle->id);
  MAKER_ASSERT((slot_index.index >= 0) && (slot_index.index < pool->pool.size));

  MKMediaPoolItem *item = _mk_media_lookup(pool, handle->id);
  if (item) {
    if (item->slot.state == MK_POOL_ITEM_STATE_VALID ||
        item->slot.state == MK_POOL_ITEM_STATE_FAILED) {

      maker_clear(&item->media, sizeof(MKMedia));
      item->slot.state = MK_POOL_ITEM_STATE_ALLOC;
    }
  }
}

MKMedia *mk_media_get(MKMediaPool *pool, MKMediaHandle *handle) {
  MKMediaPoolItem *item = _mk_media_lookup(pool, handle->id);
  if (item) {
    return &item->media;
  }
  return NULL;
}

void mk_media_free(MKMediaPool *pool, MKMediaHandle *handle) {
  mk_pool_free_item(&pool->pool, handle->id);
}
