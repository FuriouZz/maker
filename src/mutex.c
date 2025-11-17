#include "maker_internal.h"
#include <sys/time.h>

i32 mk_mutex_init(MKMutex* mutex)
{
    MK_CHECK_VALID(mutex);
    return pthread_mutex_init(&mutex->handle, NULL);
}

i32 mk_mutex_destroy(MKMutex* mutex)
{
    MK_CHECK_VALID(mutex);
    return pthread_mutex_destroy(&mutex->handle);
}

i32 mk_mutex_trylock(MKMutex* mutex)
{
    MK_CHECK_VALID(mutex);
    return pthread_mutex_trylock(&mutex->handle);
}

i32 mk_mutex_lock(MKMutex* mutex)
{
    MK_CHECK_VALID(mutex);
    return pthread_mutex_lock(&mutex->handle);
}

i32 mk_mutex_unlock(MKMutex* mutex)
{
    MK_CHECK_VALID(mutex);
    return pthread_mutex_unlock(&mutex->handle);
}

i32 mk_cond_init(MKCond* cond)
{
    MK_CHECK_VALID(cond);
    return pthread_cond_init(&cond->handle, NULL);
}

i32 mk_cond_destroy(MKCond* cond)
{
    MK_CHECK_VALID(cond);
    return pthread_cond_destroy(&cond->handle);
}

i32 mk_cond_signal(MKCond* cond)
{
    MK_CHECK_VALID(cond);
    return pthread_cond_signal(&cond->handle);
}

i32 mk_cond_broadcast(MKCond* cond)
{
    MK_CHECK_VALID(cond);
    return pthread_cond_broadcast(&cond->handle);
}

i32 mk_cond_wait(MKCond* cond, MKMutex* mutex)
{
    if (cond == NULL || mutex == NULL) {
        return -1;
    }
    return pthread_cond_wait(&cond->handle, &mutex->handle) == 0;
}

i32 mk_cond_timedwait(MKCond* cond, MKMutex* mutex, i32 seconds)
{
    if (cond == NULL || mutex == NULL) {
        return -1;
    }

    struct timeval  now     = { 0 };
    struct timespec timeout = { 0 };
    gettimeofday(&now, NULL);

    timeout.tv_sec  = now.tv_sec + seconds;
    timeout.tv_nsec = (now.tv_usec + 1000UL * seconds) * 1000UL;

    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &timeout);
}
