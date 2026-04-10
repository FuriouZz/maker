#include "maker_internal.h"

static MakerStatus maker__can_read_job(AVFifo* fifo, MakerThreadPoolJob* job, MakerMutex* mutex, MakerCond* signal, bool* is_aborted)
{
    MakerStatus status = MAKER_STATUS_ERROR;

    maker_mutex_lock(mutex);
    for (;;) {
        if (MAKER_ATOMIC_LOAD(is_aborted) == TRUE) {
            break;
        }

        if (av_fifo_can_read(fifo) > 0) {
            av_fifo_read(fifo, job, 1);
            status = MAKER_STATUS_OK;
            break;
        }

        maker_cond_wait(signal, mutex);
    }
    maker_mutex_unlock(mutex);

    return status;
}

static MakerStatus maker__thread_worker(void* user_data, u32 user_index)
{
    MakerThreadPoolContext* ctx = user_data;
    MakerThreadPoolJob      job = { 0 };

    for (;;) {
        if (MAKER_ATOMIC_LOAD(&ctx->is_aborted) == TRUE) {
            MAKER_LOG_INFO(maker_format("job aborted (%d)", user_index));
            break;
        }

        MAKER_LOG_INFO(maker_format("waiting for job (%d)", user_index));
        if (maker__can_read_job(ctx->job_queue, &job, &ctx->lock, &ctx->new_job_signal, &ctx->is_aborted) == MAKER_STATUS_OK) {
            MAKER_LOG_INFO(maker_format("job running (%d)", user_index));
            if (job.callback == NULL) {
                MAKER_LOG_WARN("Invalid worker");
            } else {
                job.callback(job.data);
            }

            job.callback = NULL;
            job.data     = NULL;
            MAKER_LOG_INFO(maker_format("job running (%d)", user_index));
        }
    }

    job.callback = NULL;
    job.data     = NULL;

    return MAKER_STATUS_OK;
}

MakerThreadPool* maker_thread_pool_alloc(void)
{
    return maker_malloc_clear(sizeof(MakerThreadPool));
}

void maker_thread_pool_dealloc(MakerThreadPool* pool)
{
    if (pool != NULL) {
        maker_free(pool);
    }
}

MakerStatus maker_thread_pool_init(MakerThreadPool* pool, usize thread_count)
{
    MAKER_ASSERT(pool);
    MAKER_ASSERT(thread_count > 0);

    maker_clear(pool, sizeof(*pool));
    pool->threads = NULL;

    MakerThreadPoolContext* context = &pool->context;
    maker_clear(context, sizeof(*context));
    context->is_aborted = FALSE;
    context->job_queue  = NULL;

    pool->threads = maker_malloc_clear(sizeof(MakerThread) * thread_count);
    if (pool->threads == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto error_end;
    }

    context->job_queue = av_fifo_alloc2(thread_count, sizeof(MakerThreadPoolJob), AV_FIFO_FLAG_AUTO_GROW);
    if (context->job_queue == NULL) {
        MAKER_OUT_OF_MEMORY;
        goto cleanup_thread_pool;
    }

    if (maker_cond_init(&context->new_job_signal) != MAKER_STATUS_OK) {
        goto cleanup_job_queue;
    }

    if (maker_mutex_init(&context->lock) != MAKER_STATUS_OK) {
        goto cleanup_cond;
    }

    for (usize i = 0; i < thread_count; i++) {
        MakerThread* thread = &pool->threads[i];
        maker_clear(thread, sizeof(*thread));
        thread->callback   = maker__thread_worker;
        thread->user_data  = context;
        thread->user_index = i;
        if (maker_thread_init(thread) != 0) {
            goto cleanup_threads;
        }
    }

    pool->count = thread_count;

    return MAKER_STATUS_OK;

cleanup_threads:
    context->is_aborted = TRUE;
    maker_cond_broadcast(&context->new_job_signal);
    for (usize i = 0; i < thread_count; i++) {
        MakerThread* thread = &pool->threads[i];
        maker_thread_wait(thread);
    }

    maker_mutex_uninit(&context->lock);

cleanup_cond:
    maker_cond_uninit(&context->new_job_signal);

cleanup_job_queue:
    av_fifo_freep2(&context->job_queue);

cleanup_thread_pool:
    maker_free(pool->threads);

error_end:
    return MAKER_STATUS_ERROR;
}

void maker_thread_pool_uninit(MakerThreadPool* pool)
{
    if (pool == NULL) return;

    bool expected = FALSE;
    if (!MAKER_ATOMIC_COMPARE_EXCHANGE(&pool->context.is_aborted, &expected, TRUE)) {
        return;
    }

    maker_cond_broadcast(&pool->context.new_job_signal);

    for (usize i = 0; i < pool->count; i++) {
        MakerThread* thread = &pool->threads[i];
        maker_thread_wait(thread);
    }

    if (pool->context.job_queue != NULL) {
        av_fifo_freep2(&pool->context.job_queue);
        pool->context.job_queue = NULL;
    }

    if (pool->threads != NULL) {
        maker_free(pool->threads);
        pool->threads = NULL;
    }
}

MakerStatus maker_thread_pool_queue_job(MakerThreadPool* pool, MakerStatus (*user_job)(void* user_data), void* user_data)
{
    MAKER_CHECK(pool);
    MAKER_CHECK(user_job);

    MakerStatus status = MAKER_STATUS_OK;

    maker_mutex_lock(&pool->context.lock);
    if (av_fifo_write(
            pool->context.job_queue,
            &(MakerThreadPoolJob) {
                .data     = user_data,
                .callback = user_job,
            },
            1
        )
        != 0) {
        MAKER_LOG_INFO("Failed to queue job.");
        status = MAKER_STATUS_ERROR;
        goto end;
    }

    maker_cond_broadcast(&pool->context.new_job_signal);

end:
    maker_mutex_unlock(&pool->context.lock);
    return status;
}

i32 maker_thread_pool_job_count(MakerThreadPool* pool)
{
    MAKER_CHECK(pool);
    return av_fifo_can_read(pool->context.job_queue);
}
