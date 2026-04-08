#define _GNU_SOURCE

#include <stdlib.h>
#include <time.h>
#ifdef __unix__
#include <sys/time.h>
#include <features.h>
#endif

#include "logging.h"

// frozen time values, read once from env
static time_t frozen_sec = 0;
static long frozen_nsec = 0;
static int frozen_init = 0;

static void detime_init(void)
{
	if (frozen_init) return;
	char *sec_str = getenv("TIME");
	char *nsec_str = getenv("TIME_NSEC");
	frozen_sec = sec_str ? atol(sec_str) : 0;
	frozen_nsec = nsec_str ? atol(nsec_str) : 0;
	frozen_init = 1;
}

time_t time(time_t *res)
{
	detime_init();
	if (res != NULL)
		*res = frozen_sec;

	preeny_debug("time frozen at %ld\n", (long)frozen_sec);
	return frozen_sec;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	detime_init();
	if (tp != NULL) {
		tp->tv_sec = frozen_sec;
		tp->tv_nsec = frozen_nsec;
	}

	preeny_debug("clock_gettime(%d) frozen at %ld.%09ld\n", clk_id, (long)frozen_sec, frozen_nsec);
	return 0;
}

#ifdef __unix__
#if !defined(__GLIBC__) || __GLIBC_PREREQ(2, 31)
int gettimeofday(struct timeval *tv, void *__restrict tz)
#else
int gettimeofday(struct timeval *tv, struct timezone *tz)
#endif
{
	detime_init();
	tv->tv_sec = frozen_sec;
	tv->tv_usec = frozen_nsec / 1000;

	preeny_debug("gettimeofday frozen at %ld.%06ld\n", (long)tv->tv_sec, (long)tv->tv_usec);

#if !defined(__GLIBC__) || __GLIBC_PREREQ(2, 31)
#else
	if (tz != NULL) {
		char *minwest_str = getenv("TZ_MINWEST");
		char *dst_str = getenv("TZ_DSTTIME");
		tz->tz_minuteswest = minwest_str ? atoi(minwest_str) : 0;
		tz->tz_dsttime = dst_str ? atoi(dst_str) : 0;
	}
#endif

	return 0;
}
#endif
