#include "thread.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

_MK_PRIVATE void *mk_thread_run(void *data) {
  MKThread *thread = data;
  MKThreadFunction fn = thread->fn;
  int *status = &thread->status;
  void *userdata = thread->userdata;
  *status = fn(userdata);
  return NULL;
}

int mk_thread_init(MKThread *thread) {
  if (thread == NULL) {
    return -1;
  }

  int ret;
  pthread_attr_t type;

  // Try to create pthread attribute
  ret = pthread_attr_init(&type);
  if (ret != 0) {
    return ret;
  }

  // Set joinable state attribute
  ret = pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);
  if (ret != 0) {
    return ret;
  }

  // Create thread
  ret = pthread_create(&thread->handle, &type, mk_thread_run, thread);

  return ret;
}

int mk_thread_wait(MKThread *thread, int *status) {
  if (thread == NULL) {
    return -1;
  }

  int ret;
  ret = pthread_join(thread->handle, 0);
  if (ret != 0) {
    return ret;
  }

  if (status != NULL) {
    *status = thread->status;
  }

  return 0;
}
