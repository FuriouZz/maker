#include "../src/maker_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_COUNT 3
#define POOL_SIZE 5
#define JOB_COUNT 20

int worker(void* data, int* aborted)
{
    (void)aborted;
    int* i = data;

    printf("worker %d\n", *i);
    sleep(2);
    printf("work %d done!\n", *i);

    free(i);

    return 0;
}

int main(void)
{
    MKThreadPool* pool = mk_thread_pool_alloc();

    printf("p %p", (void*)pool);

    printf("init %d threads with pool size %d\n", THREAD_COUNT, POOL_SIZE);
    mk_thread_pool_init(pool, THREAD_COUNT, POOL_SIZE);

    for (int i = 0; i < JOB_COUNT; i++) {
        int* value = malloc(sizeof(int));
        *value     = i + 1;
        if (mk_thread_pool_queue_job(pool, worker, value) != 0) {
            printf("failed to queue job %d\n", *value);
            free(value);
        }
    }

    while (mk_thread_pool_job_count(pool) > 0) { }
    mk_thread_pool_uninit(pool);
    mk_thread_pool_dealloc(&pool);

    return 0;
}
