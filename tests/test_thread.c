#include <maker/mutex.h>
#include <maker/thread.h>
#include <stdio.h>
#include <unistd.h>

struct {
    int value;
    MKMutex* mutex;
} state;

static int thread_a(void* data)
{
    (void)data;
    puts("hello");
    sleep(2);
    puts("world");
    if (mk_mutex_lock(state.mutex)) {
        state.value = 20;
    }

    int sum = state.value + 10;
    mk_mutex_unlock(state.mutex);

    return sum;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    int result;
    state.value = 10;
    state.mutex = mk_mutex_create();
    if (!state.mutex) {
        return -1;
    }

    MKThread* thread = mk_thread_create(thread_a, "thread_a", (void*)NULL);
    if (!thread) {
        return -1;
    }
    mk_thread_wait(thread, &result);

    printf("result=%d\n", result);

    return 0;
}
