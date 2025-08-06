#ifndef MK_THREAD_H
#define MK_THREAD_H

/**
 * Thread function callback type
 */
typedef int (*MKThreadFunction)(void* data);

/**
 * Thread object with thread informations
 */
typedef struct MKThread MKThread;

/**
 * Create a new thread
 */
MKThread* mk_thread_create(MKThreadFunction fn, const char* name, void* data);

/**
 * Wait for a thread to finish
 */
void mk_thread_wait(MKThread* thread, int* status);

/**
 * Let a thread clean up on exit without intervention
 */
void mk_thread_detach(MKThread* thread);

#endif
