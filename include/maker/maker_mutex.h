#ifndef MAKER_MUTEX_H
#define MAKER_MUTEX_H

typedef struct MKMutex MKMutex;

/**
 * Create a new mutex
 */
MKMutex *mk_mutex_create(void);

/**
 * Destroy given mutex
 */
extern void mk_mutex_destroy(MKMutex *mutex);

/**
 * Try to lock mutex
 */
extern int mk_mutex_lock(MKMutex *mutex);

/**
 * Try to lock mutex
 */
extern int mk_mutex_trylock(MKMutex *mutex);

/**
 * Unlock mutex
 */
extern int mk_mutex_unlock(MKMutex *mutex);

typedef struct MKCond MKCond;

extern MKCond *mk_cond_create(void);

extern void mk_cond_destroy(MKCond *cond);

extern void mk_cond_signal(MKCond *cond);

extern void mk_cond_broadcast(MKCond *cond);

extern void mk_cond_wait(MKCond *cond, MKMutex *mutex);

extern int mk_cond_timedwait(MKCond *cond, MKMutex *mutex, int seconds);
#endif
