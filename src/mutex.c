#include "internal.h"
#include "maker/mutex.h"
#include "maker/utils_mem.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

typedef struct MKMutex {
    pthread_mutex_t handle;
} MKMutex;

typedef struct MKCond {
    pthread_cond_t handle;
} MKCond;

MKMutex* mk_mutex_create(void)
{
    MKMutex* mutex = (MKMutex*)mk_malloc(sizeof(MKMutex));

    if (pthread_mutex_init(&mutex->handle, NULL)) {
        free(mutex);
        mutex = NULL;
    }

    return mutex;
}

void mk_mutex_destroy(MKMutex* mutex)
{
    if (!mutex)
        return;
    pthread_mutex_destroy(&mutex->handle);
    free(mutex);
}

int mk_mutex_trylock(MKMutex* mutex)
{
    if (!mutex) {
        return -1;
    }
    return pthread_mutex_trylock(&mutex->handle);
}

int mk_mutex_lock(MKMutex* mutex)
{
    if (!mutex) {
        return -1;
    }
    return pthread_mutex_lock(&mutex->handle);
}

int mk_mutex_unlock(MKMutex* mutex)
{
    if (!mutex) {
        return -1;
    }
    return pthread_mutex_unlock(&mutex->handle);
}

MKCond* mk_cond_create(void)
{
    MKCond* cond = (MKCond*)mk_malloc(sizeof(MKCond));

    if (pthread_cond_init(&cond->handle, NULL)) {
        free(cond);
        cond = NULL;
    }
    return cond;
}

void mk_cond_destroy(MKCond* cond)
{
    if (!cond)
        return;
    pthread_cond_destroy(&cond->handle);
    free(cond);
}

void mk_cond_signal(MKCond* cond)
{
    if (!cond)
        return;
    MK_ASSERT(pthread_cond_signal(&cond->handle) == 0);
}

void mk_cond_broadcast(MKCond* cond)
{
    if (!cond)
        return;
    MK_ASSERT(pthread_cond_broadcast(&cond->handle) == 0);
}

void mk_cond_wait(MKCond* cond, MKMutex* mutex)
{
    if (!cond || !mutex)
        return;
    MK_ASSERT(pthread_cond_wait(&cond->handle, &mutex->handle) == 0);
}

int mk_cond_timedwait(MKCond* cond, MKMutex* mutex, int seconds)
{
    if (!cond || !mutex)
        return -1;
    struct timeval now;
    struct timespec timeout;
    timeout.tv_sec = now.tv_sec + seconds;
    timeout.tv_nsec = now.tv_usec * 1000;
    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &timeout);
}
