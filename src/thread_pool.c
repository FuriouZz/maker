#include "maker_internal.h"

MK_PRIVATE i32 mk__thread_worker(void* data)
{
    MKThreadPoolContext* ctx = data;
    MKThreadJob          job = { 0 };

    for (;;) {
        if (ctx->aborted) {
            break;
        }

        MK_LOG_INFO("waiting for job");
        if (mk_fifo_block_read(&ctx->queue, &job, &ctx->signal, &ctx->aborted) == 1) {
            MK_LOG_DEBUG("job running");
            if (job.fn == NULL) {
                MK_LOG_WARN("Invalid worker");
            } else {
                job.fn(job.userdata);
            }
            MK_LOG_DEBUG("job completed");
        }
    }

    return 0;
}

MKThreadPool* mk_thread_pool_alloc(void)
{
    return mk_malloc(sizeof(MKThreadPool));
}

void mk_thread_pool_dealloc(MKThreadPool* pool)
{
    if (pool != NULL) {
        mk_free(pool);
    }
}

i32 mk_thread_pool_init(MKThreadPool* pool, usize thread_count, usize queue_size)
{
    pool->threads = mk_malloc(sizeof(MKThread) * thread_count);

    MKThreadPoolContext* context = mk_malloc(sizeof(MKThreadPoolContext));
    if (context == NULL) {
        goto error_context_init;
    }

    if (mk_fifo_init(&context->queue, sizeof(MKThreadJob), queue_size) != 0) {
        goto error_queue_init;
    }

    if (mk_cond_init(&context->signal) != 0) {
        goto error_signal_init;
    }

    context->aborted = FALSE;

    for (usize i = 0; i < thread_count; i++) {
        MKThread* thread = &pool->threads[i];
        thread->fn       = mk__thread_worker;
        thread->userdata = context;
        if (mk_thread_init(thread) != 0) {
            goto error_thread_init;
        }
    }

    pool->context = context;
    pool->count   = thread_count;

    return 0;

error_thread_init:
    for (size_t i = 0; i < thread_count; i++) {
        MKThread* thread = &pool->threads[i];
        mk_thread_exit(thread);
    }

error_signal_init:
    mk_fifo_uninit(&context->queue);

error_queue_init:
    mk_free(context);

error_context_init:
    mk_free(pool->threads);

    return -1;
}

i32 mk_thread_pool_uninit(MKThreadPool* pool)
{
    MK_CHECK_VALID(pool);
    pool->context->aborted = TRUE;

    mk_cond_broadcast(&pool->context->signal);
    for (size_t i = 0; i < pool->count; i++) {
        MKThread* thread = &pool->threads[i];
        mk_thread_wait(thread, NULL);
    }

    mk_fifo_uninit(&pool->context->queue);
    mk_free(pool->context);
    mk_free(pool->threads);

    return 0;
}

i32 mk_thread_pool_queue_job(MKThreadPool* pool, void (*userjob)(void* data), void* userdata)
{
    MK_CHECK_VALID(pool);
    MK_CHECK_VALID(userjob);

    if (mk_fifo_can_write(&pool->context->queue) == 0) {
        return -1;
    }

    if (mk_fifo_write(
            &pool->context->queue,
            &(MKThreadJob) {
                .userdata = userdata,
                .fn       = userjob,
            }
        )
        != 0) {
        return -1;
    }

    mk_cond_signal(&pool->context->signal);

    return 0;
}

i32 mk_thread_pool_job_count(MKThreadPool* pool)
{
    MK_CHECK_VALID(pool);
    return mk_fifo_can_read(&pool->context->queue);
}
