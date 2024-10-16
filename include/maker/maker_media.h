#ifndef MAKER_MEDIA_H
#define MAKER_MEDIA_H

#include "libavformat/avformat.h"
#include <maker/maker_pool.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct MKMedia {
  char *filename;
  AVFormatContext *format_context;
  bool is_opened;
  struct {
    bool has_stream;
    int stream_index;
    int width;
    int height;
  } video;
} MKMedia;

typedef struct MKMediaHandle {
  MKPoolSlotId id;
} MKMediaHandle;

typedef struct MKMediaPoolItem {
  MKPoolSlot slot;
  MKMedia media;
} MKMediaPoolItem;

typedef struct MKMediaPool {
  MKPool pool;
  MKMediaPoolItem *items;
} MKMediaPool;

extern bool mk_media_pool_init(MKMediaPool *pool, size_t item_count);

extern void mk_media_pool_free(MKMediaPool *pool);

extern MKMediaHandle mk_media_alloc(MKMediaPool *pool);

extern void
mk_media_init(MKMediaPool *pool, MKMediaHandle handle, char *filename);

extern MKMediaHandle mk_media_create(MKMediaPool *pool, char *filename);

extern void mk_media_uninit(MKMediaPool *pool, MKMediaHandle handle);

extern void mk_media_free(MKMediaPool *pool, MKMediaHandle handle);
#endif
