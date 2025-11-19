#include "maker_internal.h"

MakerStatus maker_mutex_init(MakerMutex* mutex)
{
    MAKER_CHECK(mutex);
    if (pthread_mutex_init(mutex, NULL) != 0) {
        MAKER_LOG_WARN("Could not initialize mutex");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

void maker_mutex_uninit(MakerMutex* mutex)
{
    if (mutex == NULL) return;
    pthread_mutex_destroy(mutex);
}

MakerStatus maker_mutex_lock(MakerMutex* mutex)
{
    MAKER_CHECK(mutex);
    if (pthread_mutex_lock(mutex) != 0) {
        MAKER_LOG_WARN("Could not lock mutex");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

MakerStatus maker_mutex_unlock(MakerMutex* mutex)
{
    MAKER_CHECK(mutex);
    if (pthread_mutex_unlock(mutex) != 0) {
        MAKER_LOG_WARN("Could not unlock mutex");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

MakerStatus maker_cond_init(MakerCond* signal)
{
    MAKER_CHECK(signal);
    if (pthread_cond_init(signal, NULL) != 0) {
        MAKER_LOG_WARN("Could not initialize signal");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

void maker_cond_uninit(MakerCond* signal)
{
    if (signal == NULL) return;
    pthread_cond_destroy(signal);
}

MakerStatus maker_cond_signal(MakerCond* signal)
{
    MAKER_CHECK(signal);
    if (pthread_cond_signal(signal) != 0) {
        MAKER_LOG_WARN("Could not trigger signal.");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

MakerStatus maker_cond_broadcast(MakerCond* signal)
{
    MAKER_CHECK(signal);
    if (pthread_cond_broadcast(signal) != 0) {
        MAKER_LOG_WARN("Could not trigger signal.");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

MakerStatus maker_cond_wait(MakerCond* signal, MakerMutex* mutex)
{
    MAKER_CHECK(signal);
    MAKER_CHECK(mutex);

    if (pthread_cond_wait(signal, mutex) != 0) {
        MAKER_LOG_WARN("Could not wait signal.");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

MakerStatus maker_cond_timedwait(MakerCond* signal, MakerMutex* mutex, i32 seconds)
{
    MAKER_CHECK(signal);
    MAKER_CHECK(mutex);

    MakerTime now = { 0 };
    maker_get_time(&now);

    MakerTime timeout = { 0 };
    timeout.tv_sec    = now.tv_sec + seconds;
    timeout.tv_nsec   = (now.tv_nsec + seconds * 1000UL);

    if (pthread_cond_timedwait(signal, mutex, &timeout) != 0) {
        MAKER_LOG_WARN("Could not wait signal");
        return MAKER_STATUS_ERROR;
    }
    return MAKER_STATUS_OK;
}

static void* maker__thread_run(void* data)
{
    MakerThread* thread = (MakerThread*)data;
    thread->status      = thread->callback(thread->userdata);
    return NULL;
}

MakerStatus maker_thread_init(MakerThread* thread)
{
    MAKER_CHECK(thread);

    pthread_attr_t type;

    // Try to create pthread attribute
    if (pthread_attr_init(&type) != 0) {
        MAKER_LOG_WARN("Could not initialize thread attribute");
        return MAKER_STATUS_ERROR;
    }

    // Set joinable state attribute
    if (pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE) != 0) {
        MAKER_LOG_WARN("Invalid attribute set");
        return MAKER_STATUS_ERROR;
    }

    // Create thread
    if (pthread_create(&thread->handle, &type, &maker__thread_run, thread) != 0) {
        MAKER_LOG_WARN("Failed to create thread");
        return MAKER_STATUS_ERROR;
    }

    return MAKER_STATUS_OK;
}

MakerStatus maker_thread_wait(MakerThread* thread)
{
    MAKER_CHECK(thread);

    pthread_join(thread->handle, 0);

    return MAKER_STATUS_OK;
}
