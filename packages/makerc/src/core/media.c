#include "maker_internal.h"

AVFormatContext* maker_media_create_context(char* url)
{
    AVFormatContext* format = avformat_alloc_context();
    if (format == NULL) {
        MAKER_OUT_OF_MEMORY;
        return NULL;
    }

    if (avformat_open_input(&format, url, NULL, NULL) != 0) {
        MAKER_LOG_WARN("Failed to open input");
        goto cleanup_context;
    }

    if (avformat_find_stream_info(format, NULL) != 0) {
        MAKER_LOG_WARN("Cannot find field info");
        goto cleanup_context;
    }

    return format;

cleanup_context:
    avformat_free_context(format);
    return NULL;
}

MakerStatus maker_media_init(MakerMedia* media, char* url)
{
    MAKER_CHECK(media);

    MakerStatus status = MAKER_STATUS_OK;

    media->format = maker_media_create_context(url);
    if (media->format == NULL) {
        goto cleanup;
    }

    status = maker_media_info_init_with_format(&media->info, media->format);
    if (status != MAKER_STATUS_OK) {
        goto cleanup;
    }

    media->is_initialized = TRUE;

    return status;

cleanup:
    maker_media_uninit(media);

    return MAKER_STATUS_ERROR;
}

void maker_media_uninit(MakerMedia* media)
{
    if (media == NULL) return;
    maker_media_info_uninit(&media->info);
    avformat_free_context(media->format);
    media->is_initialized = FALSE;
}
