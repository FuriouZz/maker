#include "maker_internal.h"

inline MakerContextInternal* maker__context_internal(MakerContext* user_context)
{
    MakerContextInternal* ctx = (MakerContextInternal*)user_context->internal_state;
    MAKER_ASSERT(ctx);
    return ctx;
}

MakerStatus maker_context_init(MakerContext* user_context, MakerContextDesc* desc)
{
    MAKER_CHECK(user_context);
    MAKER_CHECK(user_context->is_initialized == FALSE);

    MakerContextInternal* context = maker_malloc_clear(sizeof(*context));
    if (context == NULL) {
        MAKER_OUT_OF_MEMORY;
        return MAKER_STATUS_ERROR;
    }

    memcpy(&context->desc, desc, sizeof(*desc));
    if (context->desc.thread_count < 1) {
        context->desc.thread_count = 1;
    }

    if (context->desc.create_worker == NULL) {
        if (maker_thread_pool_init(&context->thread_pool, context->desc.thread_count) != MAKER_STATUS_OK) {
            goto cleanup;
        }
    }

    user_context->internal_state = context;
    user_context->is_initialized = TRUE;

    return MAKER_STATUS_OK;

cleanup:
    maker_free(context);
    return MAKER_STATUS_ERROR;
}

MakerStatus maker_context_uninit(MakerContext* user_context)
{
    MAKER_CHECK(user_context);
    MAKER_CHECK(user_context->is_initialized == TRUE);

    MakerContextInternal* context = maker__context_internal(user_context);
    maker_thread_pool_uninit(&context->thread_pool);
    maker_free(context);
    user_context->internal_state = NULL;
    user_context->is_initialized = FALSE;

    return MAKER_STATUS_OK;
}
