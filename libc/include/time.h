/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    time.h
    Time declarations.

*******************************************************************************/

#ifndef INCLUDE_TIME_H
#define INCLUDE_TIME_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(clock_t.h)
@include(size_t.h)
@include(time_t.h)
@include(clockid_t.h)
@include(timer_t.h)
@include(locale_t.h)
@include(pid_t.h)

struct sigevent;

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

__END_DECLS
#include <sortix/timespec.h>
#include <sortix/itimerspec.h>
#if defined(_SORTIX_SOURCE)
#include <sortix/tmns.h>
#endif
__BEGIN_DECLS

@include(NULL.h)

#define CLOCKS_PER_SEC 1000000l

__END_DECLS
#include <sortix/clock.h>
#include <sortix/time.h>
__BEGIN_DECLS

/* getdate_err is omitted, use strptime */

char* asctime(const struct tm*);
char* asctime_r(const struct tm* __restrict, char* __restrict);
clock_t clock(void);
/* TODO: clock_getcpuclockid */
int clock_getres(clockid_t, struct timespec*);
int clock_gettime(clockid_t, struct timespec*);
int clock_nanosleep(clockid_t, int, const struct timespec*, struct timespec*);
int clock_settime(clockid_t, const struct timespec*);
char* ctime(const time_t* clock);
char* ctime_r(const time_t* clock, char* buf);
/* ctime_r is obsolescent */
double difftime(time_t, time_t);
/* getdate is omitted, use strptime */
struct tm* gmtime(const time_t*);
struct tm* gmtime_r(const time_t* __restrict, struct tm* __restrict);
struct tm* localtime(const time_t*);
struct tm* localtime_r(const time_t* __restrict, struct tm* __restrict);
time_t mktime(struct tm*);
int nanosleep(const struct timespec*, struct timespec*);
size_t strftime(char* __restrict, size_t, const char* __restrict,
                const struct tm* __restrict);
size_t strftime_l(char* __restrict, size_t, const char* __restrict,
                const struct tm* __restrict, locale_t);
char* strptime(const char* __restrict, const char* __restrict,
               struct tm* __restrict);
time_t time(time_t*);
int timer_create(clockid_t, struct sigevent* __restrict, timer_t* __restrict);
int timer_delete(timer_t);
int timer_getoverrun(timer_t);
int timer_gettime(timer_t, struct itimerspec*);
int timer_settime(timer_t, int, const struct itimerspec* __restrict,
                  struct itimerspec* __restrict);
void tzset(void);

/* TODO: This is some _MISC_SOURCE thing according to GNU, but I like it. */
time_t timegm(struct tm*);

#if defined(_SORTIX_SOURCE)
int clock_gettimeres(clockid_t, struct timespec*, struct timespec*);
int clock_settimeres(clockid_t, const struct timespec*, const struct timespec*);
int timens(struct tmns* tmns);
#endif

extern int daylight;
extern long timezone;
extern char* tzname[];

__END_DECLS

#endif
