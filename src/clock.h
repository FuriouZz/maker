#ifndef CLOCK_H
#define CLOCK_H

#include <time.h>

typedef struct timespec MKTime;

typedef struct MKClock {
    MKTime* start_time;
    MKTime* pause_time;
} MKClock;

extern int mk_clock_init(MKClock* clock);

extern void mk_clock_free(MKClock* clock);

extern int mk_clock_start(MKClock* clock);

extern int mk_clock_pause(MKClock* clock);

extern int mk_get_time(MKTime* time);

#endif
