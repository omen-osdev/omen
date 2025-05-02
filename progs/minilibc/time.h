#ifndef _TIME_H
#define _TIME_H
typedef long time_t;
typedef long suseconds_t;
typedef long clock_t;

#define CLOCK_MONOTONIC 0

struct timeval {
    time_t tv_sec;
    suseconds_t tv_usec;
};

struct timespec {
    time_t tv_sec;
    long tv_nsec;
};

struct tm {
    int tm_sec;         /* seconds after the minute [0-60] */
    int tm_min;         /* minutes after the hour [0-59] */
    int tm_hour;        /* hours since midnight [0-23] */
    int tm_mday;        /* day of the month [1-31] */
    int tm_mon;         /* months since January [0-11] */
    int tm_year;        /* years since 1900 */
    int tm_wday;        /* days since Sunday [0-6] */
    int tm_yday;        /* days since January 1 [0-365] */
    int tm_isdst;       /* Daylight Saving Time flag */
};

struct timezone {
    int tz_minuteswest; /* minutes west of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

struct itimerval {
    struct timeval it_interval; /* timer interval */
    struct timeval it_value;    /* timer value */
};

struct itimerspec {
    struct timespec it_interval; /* timer interval */
    struct timespec it_value;    /* timer value */
};
#endif