#ifndef MAKER_MUTEX_H
#define MAKER_MUTEX_H

#include "pthread.h"

typedef struct MKMutex MKMutex;

/**
 * Create a new mutex
 */
MKMutex *mk_mutex_create(void);

/**
 * Destroy given mutex
 */
void mk_mutex_destroy(MKMutex *mutex);

/**
 * Try to lock mutex
 */
int mk_mutex_lock(MKMutex *mutex);

/**
 * Try to lock mutex
 */
int mk_mutex_trylock(MKMutex *mutex);

/**
 * Unlock mutex
 */
int mk_mutex_unlock(MKMutex *mutex);

#endif
