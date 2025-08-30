#ifndef MK_MUTEX_H
#define MK_MUTEX_H

#include <pthread.h>

typedef struct MKMutex {
    pthread_mutex_t handle;
} MKMutex;

typedef struct MKCond {
    pthread_cond_t handle;
} MKCond;

/**
 * Create a new mutex
 */
extern int mk_mutex_init(MKMutex* mutex);

/**
 * Destroy given mutex
 */
extern int mk_mutex_destroy(MKMutex* mutex);

/**
 * Try to lock mutex
 */
extern int mk_mutex_lock(MKMutex* mutex);

/**
 * Try to lock mutex
 */
extern int mk_mutex_trylock(MKMutex* mutex);

/**
 * Unlock mutex
 */
extern int mk_mutex_unlock(MKMutex* mutex);

extern int mk_cond_init(MKCond* cond);

extern int mk_cond_destroy(MKCond* cond);

extern int mk_cond_signal(MKCond* cond);

extern int mk_cond_broadcast(MKCond* cond);

extern int mk_cond_wait(MKCond* cond, MKMutex* mutex);

extern int mk_cond_timedwait(MKCond* cond, MKMutex* mutex, int seconds);

#endif
