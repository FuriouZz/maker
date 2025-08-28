#include "decoder_pool.h"
#include "maker/maker.h"
#include "maker_internal.h"
#include "media_pool.h"
#include "message_queue.h"
#include "pool.h"
#include "thread_manager.h"
#include "util.h"

MKContext2* mk_context_init(void)
{
    MKContext2* context = mk_malloc(sizeof(MKContext2));
    MK_ASSERT(context);

    mk_decoder_pool_init(&context->decoder_p);
    mk_media_pool_init(&context->media_p);
    mk_message_queue_init(&context->message_q);
    mk_thread_manager_init(&context->thread_m, &context->decoder_p, &context->message_q);

    return context;
}

MKMediaHandle mk_context_open_input(
    MKContext2* context, char* filename
)
{
    MK_ASSERT(context);

    MKMediaPool*  pool   = &context->media_p;
    MKMediaHandle handle = { 0 };
    mk_media_pool_alloc_media(pool, &handle);
    mk_media_pool_init_media(pool, &handle, &(MKMediaDesc) { .filename = filename });

    return handle;
}

void mk_context_start_decoding(MKContext2* context)
{
    MK_ASSERT(context);
    mk_thread_manager_start(&context->thread_m);
}

void mk_context_stop_decoding(MKContext2* context)
{
    MK_ASSERT(context);
    mk_thread_manager_stop(&context->thread_m);
}

MKDecoderHandle mk_context_create_decoder(MKContext2* context, MKMediaHandle* media_handle)
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

void mk_context_drop_decoder(MKContext2* context, MKDecoderHandle* decoder_handle)
{
    MK_ASSERT(context);
    MK_ASSERT(decoder_handle);

    MKDecoderPool* decoder_pool = &context->decoder_p;
    mk_decoder_pool_uninit_decoder(decoder_pool, decoder_handle);
    mk_decoder_pool_dealloc_decoder(decoder_pool, decoder_handle);

    if (mk_pool_is_empty(&context->decoder_p.pool)) {
        mk_thread_manager_stop(&context->thread_m);
    }
}

int mk_context_get_video_frame(MKContext2* context, MKDecoderHandle* handle)
{
    MK_ASSERT(context);

    MKMessageQueue* queue = &context->message_q;
    mk_message_queue_send_message(
        queue,
        &(MKMessageData) {
            .kind = MK_MESSAGEKIND_DEMUX,
            .data = {
                .handle = {
                    .slot_id = handle->slot_id,
                },
            },
        }
    );

    return 0;
}
