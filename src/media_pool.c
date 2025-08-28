#include "error.h"
#include "libavformat/avformat.h"
#include "maker/maker.h"
#include "media_pool.h"
#include "pool.h"
#include "util.h"
#include <stdio.h>

_MK_PRIVATE MKMedia2* mk__lookup_media(MKMediaPool* pool, uint32_t slot_id)
{
    uint32_t  index = mk_pool_get_index(slot_id);
    MKMedia2* media = &pool->items[index];
    if (media->slot.id == slot_id) {
        return media;
    }
    return NULL;
}

void mk_media_pool_init(MKMediaPool* pool)
{
    MK_ASSERT(pool);
    mk_pool_init(&pool->pool, MK_MAX_MEDIA_POOL_SIZE);

    pool->items = mk_malloc(sizeof(MKMedia2) * pool->pool.size);
    MK_ASSERT(pool->items);
}

void mk_media_pool_uninit(MKMediaPool* pool)
{
    MK_ASSERT(pool);

    mk_pool_uninit(&pool->pool);
    if (pool->items != NULL) {
        mk_free(pool->items);
        pool->items = NULL;
    }
}

void mk_media_pool_alloc_media(MKMediaPool* pool, MKMediaHandle* handle)
{
    uint32_t index = mk_pool_alloc_index(&pool->pool);
    if (index != 0) {
        MKMedia2* media = &pool->items[index];
        mk_pool_alloc_slot(&pool->pool, &media->slot, index);
        handle->slot_id = media->slot.id;
    } else {
        MK_WARN("Media pool is exhausted.");
    }
}

void mk_media_pool_dealloc_media(MKMediaPool* pool, MKMediaHandle* handle)
{
    MK_ASSERT(pool);

    MKMedia2* media = mk__lookup_media(pool, handle->slot_id);
    if (media != NULL) {
        mk_pool_dealloc_slot(&pool->pool, &media->slot);
        mk_clear(&media->slot, sizeof(media->slot));
    } else {
        MK_WARN("MKMediaHandle is invalid.");
    }
}

void mk_media_pool_init_media(MKMediaPool* pool, MKMediaHandle* handle, MKMediaDesc* desc)
{
    MK_ASSERT(pool);
    MK_ASSERT(handle);
    MK_ASSERT(desc);

    int       status;

    MKMedia2* media = mk__lookup_media(pool, handle->slot_id);
    if (media == NULL) {
        MK_WARN("Invalid MKMedia.\n");
        goto failed;
    }

    AVFormatContext* format = avformat_alloc_context();
    if (format == NULL) {
        MK_WARN("Failed to allocate AVFormatContext");
        goto failed;
    }

    char buf[128];
    status = avformat_open_input(&format, desc->filename, NULL, NULL);
    if (status < 0) {
        snprintf(buf, 128, "avformat_open_input() returned %d", status);
        MK_ERROR(buf);
        goto cleanup_context;
    }

    status = avformat_find_stream_info(format, NULL);
    if (status < 0) {
        snprintf(buf, 128, "avformat_find_strean_info() returned %d", status);
        MK_ERROR(buf);
        goto cleanup_context;
    }

    memset(media->streams, -1, MK_TRACK_TYPE_COUNT);

    media->streams[MK_TRACK_TYPE_VIDEO]
        = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

    media->streams[MK_TRACK_TYPE_AUDIO] = av_find_best_stream(
        format, AVMEDIA_TYPE_AUDIO, -1, media->streams[MK_TRACK_TYPE_VIDEO],
        NULL, 0
    );

    media->format     = format;
    media->slot.state = MK_RESOURCESTATE_VALID;

    return;

cleanup_context:
    avformat_free_context(format);

failed:
    media->slot.state = MK_RESOURCESTATE_FAILED;
}

void mk_media_pool_uninit_media(MKMediaPool* pool, MKMediaHandle* handle)
{
    MK_ASSERT(pool);
    MK_ASSERT(handle);

    MKMedia2* media = mk__lookup_media(pool, handle->slot_id);
    if (media != NULL) {
        avformat_free_context(media->format);
        media->slot.state = MK_RESOURCESTATE_ALLOC;
        memset(media->streams, -1, MK_TRACK_TYPE_COUNT);
    } else {
        MK_WARN("MKMediaHandle is invalid");
    }
}

MKMedia2* mk_media_pool_get_media(MKMediaPool* pool, MKMediaHandle* handle)
{
    return mk__lookup_media(pool, handle->slot_id);
}
