#ifndef MK_THREAD_H
#define MK_THREAD_H

#include <pthread.h>

/**
 * Thread function callback type
 */
typedef int (*MKThreadFunction)(void* data);

/**
 * Thread object with thread informations
 */
typedef struct MKThread {
    pthread_t handle;
    const char* name;
    int status;
    MKThreadFunction fn;
    void* userdata;
} MKThread;

/**
 * Create a new thread
 */
int mk_thread_init(MKThread* thread);

/**
 * Wait for a thread to finish
 */
int mk_thread_wait(MKThread* thread, int* status);

#endif
