#include "internal.h"
#include "maker/thread.h"
#include "maker/utils_mem.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct MKThread {
    pthread_t handle;
    const char* name;
    int status;
    MKThreadFunction fn;
    void* userdata;
} MKThread;

_MK_PRIVATE void* _mk_thread_run(void* data)
{
    MKThread* thread = data;
    MKThreadFunction fn = thread->fn;
    int* status = &thread->status;
    void* userdata = thread->userdata;
    *status = fn(userdata);
    return NULL;
}

MKThread*
mk_thread_create(MKThreadFunction fn, const char* name, void* userdata)
{
    pthread_attr_t type;

    // Try to create pthread attribute
    if (pthread_attr_init(&type) != 0) {
        return NULL;
    }

    // Create thread in joinable state
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);

    MKThread* thread = (MKThread*)mk_malloc(sizeof(*thread));
    if (!thread) {
        return NULL;
    }

    if (name) {
        thread->name = name;
    }

    thread->status = -1;
    thread->fn = fn;
    thread->userdata = userdata;

    // Create thread
    if (pthread_create(&thread->handle, &type, _mk_thread_run, thread)) {
        free(thread);
        thread = NULL;
    }

    return thread;
}

void mk_thread_wait(MKThread* thread, int* status)
{
    if (!thread)
        return;

    pthread_join(thread->handle, 0);
    if (status) {
        *status = thread->status;
    }
    free(thread);
}

void mk_thread_detach(MKThread* thread) { pthread_detach(thread->handle); }
