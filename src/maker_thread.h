#ifndef MAKER_THREAD_H
#define MAKER_THREAD_H

#include "pthread.h"

/**
 * Thread function callback type
 */
typedef int (*MKThreadFunction)(void *data);

/**
 * Thread object with thread informations
 */
typedef struct MKThread MKThread;

/**
 * Create a new thread
 */
MKThread *mk_thread_create(MKThreadFunction fn, char *name, void *data);

/**
 * Wait for a thread to finish
 */
void mk_thread_wait(MKThread *thread, int *status);

/**
 * Let a thread clean up on exit without intervention
 */
void mk_thread_detach(MKThread *thread);

#endif
