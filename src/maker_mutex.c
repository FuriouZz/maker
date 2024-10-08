#include "pthread.h"
#include "stdlib.h"

#include "maker_mutex.h"

typedef struct MKMutex {
  pthread_mutex_t handle;
} MKMutex;

MKMutex *mk_mutex_create(void) {
  MKMutex *mutex = (MKMutex *)calloc(1, sizeof(MKMutex));

  if (pthread_mutex_init(&mutex->handle, NULL)) {
    free(mutex);
    mutex = NULL;
  }

  return mutex;
}

void mk_mutex_destroy(MKMutex *mutex) {
  if (!mutex)
    return;
  pthread_mutex_destroy(&mutex->handle);
  free(mutex);
}

int mk_mutex_trylock(MKMutex *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_trylock(&mutex->handle);
}

int mk_mutex_lock(MKMutex *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_lock(&mutex->handle);
}

int mk_mutex_unlock(MKMutex *mutex) {
  if (!mutex) {
    return -1;
  }
  return pthread_mutex_unlock(&mutex->handle);
}
