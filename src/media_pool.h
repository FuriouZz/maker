#ifndef MK_MEDIA_POOL_H
#define MK_MEDIA_POOL_H

#include "libavformat/avformat.h"
#include "maker/maker.h"
#include "pool.h"

#define MK_MAX_MEDIA_POOL_SIZE 20

typedef struct MKMedia2 {
    AVFormatContext* format;
    MKPoolSlot       slot;
    int              streams[MK_TRACK_TYPE_COUNT];
    char*            filename;
} MKMedia2;

typedef struct MKMediaPool {
    MKMedia2* items;
    MKPool    pool;
} MKMediaPool;

extern void      mk_media_pool_init(MKMediaPool* pool);
extern void      mk_media_pool_uninit(MKMediaPool* pool);
extern void      mk_media_pool_alloc_media(MKMediaPool* pool, MKMediaHandle* handle);
extern void      mk_media_pool_dealloc_media(MKMediaPool* pool, MKMediaHandle* handle);
extern void      mk_media_pool_init_media(MKMediaPool* pool, MKMediaHandle* handle, MKMediaDesc* desc);
extern void      mk_media_pool_uninit_media(MKMediaPool* pool, MKMediaHandle* handle);
extern MKMedia2* mk_media_pool_get_media(MKMediaPool* pool, MKMediaHandle* handle);

#endif
