#include <maker/maker_media.h>

#include <maker/maker_util.h>

#include <stdio.h>
#include <string.h>

#include "maker/maker_pool.h"
#include "maker_internal.h"
#include "maker_pool_c.h"

_MAKER_PRIVATE MKMediaPoolItem *
_mk_media_lookup(MKMediaPool *pool, uint32_t slot_id) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  uint32_t slot_index = mk_pool_slot_index(slot_id);
  MAKER_ASSERT((slot_index > 0) && (slot_index < pool->pool.size));

  MKMediaPoolItem *item = &pool->items[slot_index];

  if (item->slot.id == slot_id) {
    return item;
  }

  return NULL;
}

MKMedia mk_media_open(char *filename) {
  MKMedia media =
      {.filename = strdup(filename),
       .video = {
           .has_stream = false,
           .stream_index = -1,
           .width = -1,
           .height = -1,
       }};
  return media;
}

void mk_media_pool_init(MKMediaPool *pool, size_t item_count) {
  MAKER_ASSERT(mk_pool_init(&pool->pool, item_count));
  pool->items = maker_malloc(pool->pool.size * sizeof(MKMediaPoolItem));

  if (!pool->items || !pool->pool.valid) {
    maker_free(pool->items);
  }
}

void mk_media_pool_free(MKMediaPool *pool) {
  mk_pool_discard(&pool->pool);
  maker_free(pool->items);
}

MKMediaHandle mk_media_alloc(MKMediaPool *pool) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  uint32_t slot_index = mk_pool_item_alloc_index(&pool->pool);
  MKMediaHandle handle = {.id = 0};
  if (slot_index != 0) {
    handle.id = mk_pool_item_alloc(
        &pool->pool, &pool->items[slot_index].slot, slot_index
    );
  }

  return handle;
}

void mk_media_init(MKMediaPool *pool, MKMediaHandle handle, char *filename) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  uint32_t slot_index = mk_pool_slot_index(handle.id);

  printf("slot_index %d %d", slot_index, pool->pool.size);
  MAKER_ASSERT((slot_index > 0) && (slot_index < pool->pool.size));

  MKMediaPoolItem *item = _mk_media_lookup(pool, handle.id);
  if (item) {
    if (item->slot.state == MK_POOL_ITEM_STATE_ALLOC) {
      item->media = (MKMedia){.filename = strdup(filename)};
      item->slot.state = MK_POOL_ITEM_STATE_VALID;
    }
  }
}

MKMediaHandle mk_media_create(MKMediaPool *pool, char *filename) {
  MKMediaHandle handle = mk_media_alloc(pool);
  mk_media_init(pool, handle, filename);
  return handle;
}

void mk_media_uninit(MKMediaPool *pool, MKMediaHandle handle) {
  MAKER_ASSERT(&(pool->pool) && pool->pool.valid);

  uint32_t slot_index = mk_pool_slot_index(handle.id);
  MAKER_ASSERT((slot_index >= 0) && (slot_index < pool->pool.size));

  MKMediaPoolItem *item = _mk_media_lookup(pool, handle.id);
  if (item) {
    if (item->slot.state == MK_POOL_ITEM_STATE_VALID ||
        item->slot.state == MK_POOL_ITEM_STATE_FAILED) {

      maker_clear(&item->media, sizeof(MKMedia));
      item->slot.state = MK_POOL_ITEM_STATE_ALLOC;
    }
  }
}

void mk_media_free(MKMediaPool *pool, MKMediaHandle handle) {
  mk_pool_item_free(&pool->pool, handle.id);
}
