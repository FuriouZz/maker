#include "mutex.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <time.h>

int mk_mutex_init(MKMutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    return pthread_mutex_init(&mutex->handle, NULL);
}

int mk_mutex_destroy(MKMutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    return pthread_mutex_destroy(&mutex->handle);
}

int mk_mutex_trylock(MKMutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    return pthread_mutex_trylock(&mutex->handle);
}

int mk_mutex_lock(MKMutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    return pthread_mutex_lock(&mutex->handle);
}

int mk_mutex_unlock(MKMutex* mutex)
{
    if (mutex == NULL) {
        return -1;
    }
    return pthread_mutex_unlock(&mutex->handle);
}

int mk_cond_init(MKCond* cond)
{
    if (cond == NULL) {
        return -1;
    }
    return pthread_cond_init(&cond->handle, NULL);
}

int mk_cond_destroy(MKCond* cond)
{
    if (cond == NULL) {
        return -1;
    }
    return pthread_cond_destroy(&cond->handle);
}

int mk_cond_signal(MKCond* cond)
{
    if (cond == NULL) {
        return -1;
    }
    return pthread_cond_signal(&cond->handle);
}

int mk_cond_broadcast(MKCond* cond)
{
    if (cond == NULL) {
        return -1;
    }
    return pthread_cond_broadcast(&cond->handle);
}

int mk_cond_wait(MKCond* cond, MKMutex* mutex)
{
    if (cond == NULL || mutex == NULL) {
        return -1;
    }
    return pthread_cond_wait(&cond->handle, &mutex->handle) == 0;
}

int mk_cond_timedwait(MKCond* cond, MKMutex* mutex, int seconds)
{
    if (cond == NULL || mutex == NULL) {
        return -1;
    }

    struct timeval now = { 0 };
    struct timespec timeout = { 0 };
    gettimeofday(&now, NULL);

    timeout.tv_sec = now.tv_sec + seconds;
    timeout.tv_nsec = (now.tv_usec + 1000UL * seconds) * 1000UL;

    return pthread_cond_timedwait(&cond->handle, &mutex->handle, &timeout);
}
