#include "../src/maker_mutex.h"
#include "../src/maker_thread.h"
#include <stdio.h>
#include <unistd.h>

struct {
  int value;
  MKMutex *mutex;
} state;

static int thread_a(void *data) {
  puts("hello");
  sleep(2);
  puts("world");
  if (mk_mutex_trylock(state.mutex)) {
    state.value = 20;
  }

  int sum = state.value + 10;
  mk_mutex_unlock(state.mutex);

  return sum;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  int status;
  state.value = 10;
  state.mutex = mk_mutex_create();

  MKThread *thread = mk_thread_create(thread_a, "thread_a", (void *)NULL);
  mk_thread_wait(thread, &status);

  printf("status=%d\n", status);

  return 0;
}
