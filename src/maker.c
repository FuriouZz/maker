#include "maker/maker.h"
#include "maker_internal.h"
#include <stdio.h>

MKContext* mk_context_init(void)
{
    MKContext* context = mk_malloc(sizeof(MKContext));
    MK_ASSERT(context);

    mk_thread_pool_init(&context->pool, 4, 16);
    mk_decoder_pool_init(&context->decoder_p);
    mk_media_pool_init(&context->media_p);

    context->demux_context.decoder.slot_id  = 0;
    context->demux_context.decoder_p        = &context->decoder_p;
    context->decode_context.decoder.slot_id = 0;
    context->decode_context.decoder_p       = &context->decoder_p;

    return context;
}

void mk_context_uninit(MKContext* context)
{
    MK_ASSERT(context);

    mk_thread_pool_uninit(&context->pool);
    mk_decoder_pool_uninit(&context->decoder_p);
    mk_media_pool_uninit(&context->media_p);

    mk_free(context);
}

MKMediaHandle mk_context_open_input(MKContext* context, char* filename)
{
    MK_ASSERT(context);

    MKMediaPool*  pool   = &context->media_p;
    MKMediaHandle handle = { 0 };
    mk_media_pool_alloc_media(pool, &handle);
    mk_media_pool_init_media(pool, &handle, &(MKMediaDesc) { .filename = filename });

    return handle;
}

// int32 mk_context_start_playback(MKContext* ctx)
// {
//     if (ctx == NULL) {
//         return -1;
//     }

//     MKInternalContext* context = ctx->context;
//     mk_clock_start(&context->clock);

//     return 0;
// }

// int32 mk_context_pause_playback(MKContext* ctx)
// {
//     if (ctx == NULL) {
//         return -1;
//     }

//     MKInternalContext* context = ctx->context;
//     mk_clock_pause(&context->clock);

//     return 0;
// }

// int32 mk_context_get_playback_time(MKContext* ctx, int32* time_ms)
// {
//     if (ctx == NULL) {
//         return -1;
//     }
//     if (time_ms == NULL) {
//         return -1;
//     }

//     int32              status;
//     MKInternalContext* context = ctx->context;
//     MKClock*           clock   = &context->clock;

//     MKTime time;
//     status = mk_get_time(&time);
//     if (status != 0) {
//         return -1;
//     };

//     *time_ms = (time.tv_sec - clock->start_time->tv_sec) * 1000
//         + (time.tv_nsec - clock->start_time->tv_nsec) / 1000000;

//     return 0;
// }

// int32 mk_context_set_playback_time(MKContext* ctx, int time_ms)
// {
//     if (ctx == NULL) {
//         return -1;
//     }

//     int                status;
//     MKInternalContext* context = ctx->context;
//     MKClock*           clock   = &context->clock;

//     int time_s  = time_ms / 1000;
//     int time_ns = (time_ms - ((time_ms / 1000) * 1000)) * 1000000;

//     MKTime time;
//     status = mk_get_time(&time);
//     if (status != 0) {
//         return -1;
//     };

//     clock->start_time->tv_sec  = time.tv_sec - time_s;
//     clock->start_time->tv_nsec = time.tv_nsec - time_ns;

//     return 0;
// }

MKDecoderHandle mk_context_create_decoder(MKContext* context, MKMediaHandle* media_handle)
{
    MK_ASSERT(context);
    MK_ASSERT(media_handle);

    MKMediaPool* pool  = &context->media_p;
    MKMedia2*    media = mk_media_pool_get_media(pool, media_handle);
    MK_ASSERT(media);

    MKDecoderPool*  decoder_pool   = &context->decoder_p;
    MKDecoderHandle decoder_handle = mk_decoder_pool_alloc_decoder(decoder_pool);
    mk_decoder_pool_init_decoder(decoder_pool, &decoder_handle, media);

    return decoder_handle;
}

void mk_context_drop_decoder(MKContext* context, MKDecoderHandle* decoder_handle)
{
    MK_ASSERT(context);
    MK_ASSERT(decoder_handle);

    MKDecoderPool* decoder_pool = &context->decoder_p;
    mk_decoder_pool_uninit_decoder(decoder_pool, decoder_handle);
    mk_decoder_pool_dealloc_decoder(decoder_pool, decoder_handle);
}

MK_PRIVATE void mk__context_demux(void* data)
{
    MKDecoderData*  ctx     = (MKDecoderData*)data;
    MKDecoderPool*  pool    = ctx->decoder_p;
    MKDecoderHandle decoder = { .slot_id = ctx->decoder.slot_id };

    mk_decoder_pool_demux(pool, &decoder, NULL);
    if (ctx->complete_signal) mk_cond_signal(ctx->complete_signal);
}

MK_PRIVATE void mk__context_decode_video(void* data)
{
    MKDecoderData*  ctx     = (MKDecoderData*)data;
    MKDecoderPool*  pool    = ctx->decoder_p;
    MKDecoderHandle decoder = { .slot_id = ctx->decoder.slot_id };

    mk_decoder_pool_decode_video(pool, &decoder, NULL);
    if (ctx->complete_signal) mk_cond_signal(ctx->complete_signal);
}

int mk_context_get_video_frame(MKContext* context, MKDecoderHandle* handle)
{
    MK_CHECK_VALID(context);
    MK_CHECK_VALID(handle);

    MKMutex        mutex = { 0 };
    MKDecoderData* data  = NULL;

    i32 has_video_packets = mk_decoder_has_video_packets(&context->decoder_p, handle);
    i32 has_video_frames  = mk_decoder_has_video_frames(&context->decoder_p, handle);

    if (has_video_packets <= 0 || has_video_frames <= 0) {
        data                  = mk_malloc(sizeof(*data));
        data->decoder_p       = &context->decoder_p;
        data->decoder.slot_id = handle->slot_id;

        data->complete_signal = mk_malloc(sizeof(*data->complete_signal));
        if (mk_cond_init(data->complete_signal) < 0) {
            goto cleanup;
        }

        if (mk_mutex_init(&mutex) < 0) {
            goto cleanup;
        }
    }

    if (has_video_packets <= 0) {
        mk_thread_pool_queue_job(&context->pool, mk__context_demux, data);

        mk_mutex_lock(&mutex);
        mk_cond_wait(data->complete_signal, &mutex);
        mk_mutex_unlock(&mutex);
    }

    if (has_video_frames <= 0) {
        mk_thread_pool_queue_job(&context->pool, mk__context_decode_video, data);

        mk_mutex_lock(&mutex);
        mk_cond_wait(data->complete_signal, &mutex);
        mk_mutex_unlock(&mutex);
    }

cleanup:
    if (data) {
        if (data->complete_signal) {
            mk_cond_destroy(data->complete_signal);
            mk_free(data->complete_signal);
        }
        mk_free(data);
        mk_mutex_destroy(&mutex);
    }

    MK_LOG_DEBUG("Ready to get frame");

    return 0;
}
