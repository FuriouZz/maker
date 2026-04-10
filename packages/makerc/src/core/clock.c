#include "maker_internal.h"

MakerStatus maker_get_time(MakerTime* time)
{
    MAKER_CHECK(time);

    if (clock_gettime(CLOCK_MONOTONIC, time) != 0) {
        return MAKER_STATUS_ERROR;
    };

    return MAKER_STATUS_OK;
}

MakerStatus maker_clock_init(MakerClock* user_clock)
{
    MAKER_CHECK(user_clock);
    MAKER_CHECK(!user_clock->is_initialized);

    MakerClockInternal* clock = maker_malloc_clear(sizeof(*clock));
    if (clock == NULL) {
        MAKER_OUT_OF_MEMORY;
        return MAKER_STATUS_ERROR;
    }

    clock->start_time = maker_malloc_clear(sizeof(*clock->start_time));

    if (clock->start_time == NULL) {
        MAKER_OUT_OF_MEMORY;
        maker_free(clock);
        return MAKER_STATUS_ERROR;
    }

    user_clock->is_initialized = TRUE;
    user_clock->internal_state = clock;

    return MAKER_STATUS_OK;
}

void maker_clock_uninit(MakerClock* user_clock)
{
    if (user_clock == NULL) return;

    MakerClockInternal* clock = (MakerClockInternal*)user_clock->internal_state;
    maker_free(clock->start_time);
    maker_free(clock->pause_time);
    maker_free(clock);

    user_clock->internal_state = NULL;
    user_clock->is_initialized = FALSE;
}

MakerStatus maker_clock_start(MakerClock* user_clock)
{
    MAKER_CHECK(user_clock);

    MakerClockInternal* clock = (MakerClockInternal*)user_clock->internal_state;
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

MakerStatus maker_clock_pause(MakerClock* user_clock)
{
    MAKER_CHECK(user_clock);
    MakerClockInternal* clock = (MakerClockInternal*)user_clock->internal_state;

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

MakerStatus maker_clock_get_time(MakerClock* user_clock, u32* time_ms)
{
    MAKER_CHECK(user_clock);
    MAKER_CHECK(user_clock->is_initialized);
    MAKER_CHECK(time_ms);

    MakerClockInternal* clock = (MakerClockInternal*)user_clock->internal_state;

    MakerTime time = { 0 };
    if (maker_get_time(&time) != MAKER_STATUS_OK) {
        return MAKER_STATUS_ERROR;
    };

    *time_ms = (time.tv_sec - clock->start_time->tv_sec) * 1000
        + (time.tv_nsec - clock->start_time->tv_nsec) / 1000000;

    return MAKER_STATUS_OK;
}
