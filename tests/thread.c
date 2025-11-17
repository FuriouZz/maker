#include "../src/maker_internal.h"
#include <stdio.h>
#include <unistd.h>

struct {
    int     value;
    MKMutex mutex;
} state;

static int thread_a(void* data)
{
    (void)data;
    puts("hello");
    sleep(2);
    puts("world");
    if (mk_mutex_lock(&state.mutex)) {
        state.value = 20;
    }

    int sum = state.value + 10;
    mk_mutex_unlock(&state.mutex);

    return sum;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    int ret;
    int result;
    state.value = 10;
    ret         = mk_mutex_init(&state.mutex);
    if (ret != 0) {
        return ret;
    }

    MKThread thread = {
        .name = "thread_a",
        .fn   = thread_a,
    };
    ret = mk_thread_init(&thread);
    if (ret != 0) {
        return ret;
    }
    mk_thread_wait(&thread, &result);

    printf("result=%d\n", result);

    return 0;
}
