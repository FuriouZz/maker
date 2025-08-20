#include "clock.h"
#include "util.h"
#include <_time.h>
#include <time.h>

int mk_clock_init(MKClock* clock)
{
    MK_CHECK_VALID(clock);
    clock->start_time = mk_malloc(sizeof(struct timespec));
    return 0;
}

void mk_clock_free(MKClock* clock)
{
    if (clock != NULL) {
        mk_free(clock->start_time);
        mk_free(clock->pause_time);
    }
}

int mk_clock_start(MKClock* clock)
{
    MK_CHECK_VALID(clock);

    int ret;

    if (clock->pause_time == NULL) {
        ret = clock_gettime(CLOCK_MONOTONIC, clock->start_time);
        MK_CHECK_RESULT(ret);
    } else {
        MKTime time;

        ret = clock_gettime(CLOCK_MONOTONIC, &time);
        MK_CHECK_RESULT(ret);

        clock->start_time->tv_sec += time.tv_sec - clock->pause_time->tv_sec;
        clock->start_time->tv_sec
            += (time.tv_nsec - clock->pause_time->tv_nsec) / 1000000000;
        clock->start_time->tv_nsec
            += (time.tv_nsec - clock->pause_time->tv_nsec) % 1000000000;

        mk_free(clock->pause_time);
        clock->pause_time = NULL;
    }

    return 0;
}

int mk_clock_pause(MKClock* clock)
{
    MK_CHECK_VALID(clock);

    int ret;

    if (clock->pause_time == NULL) {
        clock->pause_time = mk_malloc(sizeof(struct timespec));
        ret = clock_gettime(CLOCK_MONOTONIC, clock->pause_time);
        MK_CHECK_RESULT(ret);
    }

    return 0;
}

int mk_get_time(MKTime* time)
{
    MK_CHECK_VALID(time);

    int ret = clock_gettime(CLOCK_MONOTONIC, time);
    MK_CHECK_RESULT(ret);

    return 0;
}
