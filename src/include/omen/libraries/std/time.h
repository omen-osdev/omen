#ifndef _TIME_H_
#define _TIME_H_

#define CLOCK_MONOTONIC 0

#define DST_NONE    0   /* not on dst */
#define DST_USA     1   /* USA style dst */
#define DST_AUST    2   /* Australian style dst */
#define DST_WET     3   /* Western European dst */
#define DST_MET     4   /* Middle European dst */
#define DST_EET     5   /* Eastern European dst */
#define DST_CAN     6   /* Canada */

typedef long time_t;
typedef long suseconds_t;

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


#define	TIMEVAL_TO_TIMESPEC(tv, ts) do {				\
	(ts)->tv_sec = (tv)->tv_sec;					\
	(ts)->tv_nsec = (tv)->tv_usec * 1000;				\
} while (0)
#define	TIMESPEC_TO_TIMEVAL(tv, ts) do {				\
	(tv)->tv_sec = (ts)->tv_sec;					\
	(tv)->tv_usec = (ts)->tv_nsec / 1000;				\
} while (0)

struct timeval * timeval_now(struct timeval *tv);
struct timespec * timespec_now(struct timespec *ts);
struct timespec * clock_res(struct timespec *ts);
#endif