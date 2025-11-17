#include "maker_internal.h"

MakerStatus maker_get_time(MakerTime* time)
{
    MAKER_CHECK(time);

    if (clock_gettime(CLOCK_MONOTONIC, time) != 0) {
        return MAKER_STATUS_ERROR;
    };

    return MAKER_STATUS_OK;
}

MakerStatus maker_clock_init(MakerClock* clock)
{
    MAKER_CHECK(clock);

    maker_clear(clock, sizeof(*clock));
    clock->start_time = maker_malloc_clear(sizeof(*clock->start_time));

    if (clock->start_time == NULL) {
        MAKER_OUT_OF_MEMORY;
        return MAKER_STATUS_ERROR;
    }

    return MAKER_STATUS_OK;
}

void maker_clock_uninit(MakerClock* clock)
{
    if (clock == NULL) return;

    maker_free(clock->start_time);
    maker_free(clock->pause_time);
}

MakerStatus maker_clock_start(MakerClock* clock)
{
    MAKER_CHECK(clock);

    if (clock->pause_time == NULL) {
        return maker_get_time(clock->start_time);
    }

    MakerTime time;

    if (maker_get_time(&time) != MAKER_STATUS_OK) {
        return MAKER_STATUS_ERROR;
    };

    clock->start_time->tv_sec += time.tv_sec - clock->pause_time->tv_sec;
    clock->start_time->tv_sec
        += (time.tv_nsec - clock->pause_time->tv_nsec) / 1000000000;
    clock->start_time->tv_nsec
        += (time.tv_nsec - clock->pause_time->tv_nsec) % 1000000000;

    maker_free(clock->pause_time);
    clock->pause_time = NULL;

    return MAKER_STATUS_OK;
}

MakerStatus maker_clock_pause(MakerClock* clock)
{
    MAKER_CHECK(clock);

    if (clock->pause_time == NULL) {
        clock->pause_time = maker_malloc_clear(sizeof(*clock->pause_time));
        if (clock->pause_time == NULL) {
            MAKER_OUT_OF_MEMORY;
            return MAKER_STATUS_ERROR;
        }
        return maker_get_time(clock->pause_time);
    }

    return MAKER_STATUS_OK;
}
