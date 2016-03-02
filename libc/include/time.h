/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * time.h
 * Time declarations.
 */

#ifndef INCLUDE_TIME_H
#define INCLUDE_TIME_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __clock_t_defined
#define __clock_t_defined
typedef __clock_t clock_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifndef __clockid_t_defined
#define __clockid_t_defined
typedef __clockid_t clockid_t;
#endif

#ifndef __timer_t_defined
#define __timer_t_defined
typedef __timer_t timer_t;
#endif

#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

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

#ifdef __cplusplus
} /* extern "C" */
#endif
#include <sortix/timespec.h>
#include <sortix/itimerspec.h>
#if __USE_SORTIX
#include <sortix/tmns.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#define CLOCKS_PER_SEC 1000000l

#ifdef __cplusplus
} /* extern "C" */
#endif
#include <sortix/clock.h>
#include <sortix/time.h>
#ifdef __cplusplus
extern "C" {
#endif

/* getdate_err is omitted, use strptime */

#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("asctime() is obsolete, use strftime()")))
#endif
char* asctime(const struct tm*);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("asctime_r() is obsolete, use strftime()")))
#endif
char* asctime_r(const struct tm* __restrict, char* __restrict);
clock_t clock(void);
/* TODO: clock_getcpuclockid */
int clock_getres(clockid_t, struct timespec*);
int clock_gettime(clockid_t, struct timespec*);
int clock_nanosleep(clockid_t, int, const struct timespec*, struct timespec*);
int clock_settime(clockid_t, const struct timespec*);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("ctime() is obsolete, use strftime()")))
#endif
char* ctime(const time_t* clock);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("ctime_r() is obsolete, use strftime()")))
#endif
char* ctime_r(const time_t* clock, char* buf);
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

#if __USE_SORTIX
int clock_gettimeres(clockid_t, struct timespec*, struct timespec*);
int clock_settimeres(clockid_t, const struct timespec*, const struct timespec*);
int timens(struct tmns* tmns);
#endif

extern char* tzname[2];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
