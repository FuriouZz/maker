#include "maker_internal.h"

MakerStatus maker_context_init(MakerContext* user_context, MakerContextDesc* desc)
{
    MAKER_CHECK(user_context);
    MAKER_CHECK(user_context->is_initialized == FALSE);

    MakerContextInternal* context = maker_malloc_clear(sizeof(*context));
    if (context == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup;
    }

    memcpy(&context->desc, desc, sizeof(*desc));
    MAKER_ASSERT(context->desc.thread_count >= 1);

    if (context->desc.create_worker == NULL) {
        if (maker_thread_pool_init(&context->thread_pool, context->desc.thread_count) != MAKER_STATUS_OK) {
            goto cleanup;
        }
    }

    user_context->internal_state = context;
    user_context->is_initialized = TRUE;

    return MAKER_STATUS_OK;

cleanup:
    maker_thread_pool_uninit(&context->thread_pool);
    maker_free(context);
    return MAKER_STATUS_ERROR;
}

void maker_context_uninit(MakerContext* user_context)
{
    if (user_context == NULL || user_context->is_initialized == FALSE) return;

    MakerContextInternal* context = (MakerContextInternal*)user_context->internal_state;
    maker_thread_pool_uninit(&context->thread_pool);
    maker_free(context);
    maker_clear(user_context, sizeof(*user_context));
}
