#include "maker_internal.h"

MK_PRIVATE void* mk_thread_run(void* data)
{
    MKThread*        thread   = data;
    MKThreadFunction fn       = thread->fn;
    int*             status   = &thread->status;
    void*            userdata = thread->userdata;
    *status                   = fn(userdata);
    return NULL;
}

i32 mk_thread_init(MKThread* thread)
{
    if (thread == NULL) {
        return -1;
    }

    i32            ret;
    pthread_attr_t type;

    // Try to create pthread attribute
    ret = pthread_attr_init(&type);
    if (ret != 0) {
        return -1;
    }

    // Set joinable state attribute
    ret = pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);
    if (ret != 0) {
        return -1;
    }

    // Create thread
    ret = pthread_create(&thread->handle, &type, mk_thread_run, thread);

    return 0;
}

i32 mk_thread_wait(MKThread* thread, i32* status)
{
    if (thread == NULL) {
        return -1;
    }

    i32 ret;
    ret = pthread_join(thread->handle, 0);
    if (ret != 0) {
        return -1;
    }

    if (status != NULL) {
        *status = thread->status;
    }

    return 0;
}

i32 mk_thread_exit(MKThread* thread)
{
    if (thread == NULL) {
        return -1;
    }

    pthread_exit(thread->handle);

    return 0;
}
