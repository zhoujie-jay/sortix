/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * wchar.h
 * Wide character support.
 */

#ifndef INCLUDE_WCHAR_H
#define INCLUDE_WCHAR_H

#include <sys/cdefs.h>

#include <__/pthread.h>
#include <sys/__/types.h>

#include <__/wchar.h>

#if !(__USE_SORTIX || __USE_POSIX)
/* Intentional namespace pollution due to ISO C stupidity: */
#endif
/* TODO: This exposes more than just the required va_list. */
#include <stdarg.h>

#if defined(__is_sortix_libc)
#include <FILE.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __wchar_t_defined
#define __wchar_t_defined
#define __need_wchar_t
#include <stddef.h>
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __FILE_defined
#define __FILE_defined
#if !(__USE_SORTIX || __USE_POSIX)
/* Intentional namespace pollution due to ISO C stupidity: */
#endif
typedef struct __FILE FILE;
#endif

#if __USE_SORTIX || 200809L <= __USE_POSIX
#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif
#endif

#ifndef __wint_t_defined
#define __wint_t_defined
typedef __wint_t wint_t;
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN __WCHAR_MIN
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX
#endif

#ifndef WEOF
#define WEOF __WEOF
#endif

#ifndef __mbstate_t_defined
/* Conversion state information. */
typedef struct
{
#if defined(__is_sortix_libc)
	unsigned short count;
	unsigned short length;
	wint_t wch;
#else
	unsigned short __count;
	unsigned short __length;
	wint_t __wch;
#endif
} mbstate_t;
#define __mbstate_t_defined 1
#endif

struct tm;

wint_t btowc(int);
/* TODO: wint_t fgetwc(FILE*); */
/* TODO: wchar_t* fgetws(wchar_t* __restrict, int, FILE* __restrict); */
/* TODO: wint_t fputwc(wchar_t, FILE*); */
/* TODO: int fputws(const wchar_t* __restrict, FILE* __restrict); */
/* TODO: wint_t getwc(FILE*); */
/* TODO: wint_t getwchar(void); */
size_t mbrlen(const char* __restrict, size_t, mbstate_t* __restrict);
size_t mbrtowc(wchar_t* __restrict, const char* __restrict, size_t, mbstate_t* __restrict);
int mbsinit(const mbstate_t*);
size_t mbsrtowcs(wchar_t* __restrict, const char** __restrict, size_t, mbstate_t* __restrict);
/* TODO: wint_t putwc(wchar_t, FILE*); */
/* TODO: wint_t putwchar(wchar_t); */
/* TODO: wint_t ungetwc(wint_t, FILE*); */
size_t wcrtomb(char* __restrict, wchar_t, mbstate_t* __restrict);
wchar_t* wcscat(wchar_t* __restrict, const wchar_t* __restrict);
wchar_t* wcschr(const wchar_t*, wchar_t);
int wcscmp(const wchar_t*, const wchar_t*);
int wcscoll(const wchar_t*, const wchar_t*);
wchar_t* wcscpy(wchar_t* __restrict, const wchar_t* __restrict);
size_t wcscspn(const wchar_t*, const wchar_t*);
size_t wcslen(const wchar_t*);
wchar_t* wcsncat(wchar_t* __restrict, const wchar_t* __restrict, size_t);
int wcsncmp(const wchar_t*, const wchar_t*, size_t);
wchar_t* wcsncpy(wchar_t* __restrict, const wchar_t* __restrict, size_t);
wchar_t* wcspbrk(const wchar_t*, const wchar_t*);
wchar_t* wcsrchr(const wchar_t*, wchar_t);
size_t wcsrtombs(char* __restrict, const wchar_t** __restrict, size_t, mbstate_t* __restrict);
size_t wcsspn(const wchar_t*, const wchar_t*);
wchar_t* wcsstr(const wchar_t* __restrict, const wchar_t* __restrict);
double wcstod(const wchar_t* __restrict, wchar_t** __restrict);
wchar_t* wcstok(wchar_t* __restrict, const wchar_t* __restrict, wchar_t** __restrict);
long wcstol(const wchar_t* __restrict, wchar_t** __restrict, int);
unsigned long wcstoul(const wchar_t* __restrict, wchar_t** __restrict, int);
size_t wcsxfrm(wchar_t* __restrict, const wchar_t* __restrict, size_t);
int wctob(wint_t);
wchar_t* wmemchr(const wchar_t*, wchar_t, size_t);
int wmemcmp(const wchar_t*, const wchar_t*, size_t);
wchar_t* wmemcpy(wchar_t* __restrict, const wchar_t* __restrict, size_t);
wchar_t* wmemmove(wchar_t*, const wchar_t*, size_t);
wchar_t* wmemset(wchar_t*, wchar_t, size_t);

#if __USE_SORTIX || 1995 <= __USE_C || 500 <= __USE_XOPEN
/* TODO: int fwide(FILE*, int); */
/* TODO: int fwprintf(FILE* __restrict, const wchar_t* __restrict, ...); */
/* TODO: int fwscanf(FILE* __restrict, const wchar_t* __restrict, ...); */
/* TODO: int swprintf(wchar_t* __restrict, size_t, const wchar_t* __restrict, ...); */
/* TODO: int swscanf(const wchar_t* __restrict, const wchar_t* __restrict, ...); */
/* TODO: int vfwprintf(FILE* __restrict, const wchar_t* __restrict, va_list); */
/* TODO: int vswprintf(wchar_t* __restrict, size_t, const wchar_t* __restrict, va_list); */
/* TODO: int vwprintf(const wchar_t* __restrict, va_list); */
/* TODO: int wprintf(const wchar_t* __restrict, ...); */
/* TODO: int wscanf(const wchar_t* __restrict, ...); */
#endif

/* Functions from C99. */
#if __USE_SORTIX || 1999 <= __USE_C
/* TODO: int vfwscanf(FILE* __restrict, const wchar_t* __restrict, va_list); */
/* TODO: int vswscanf(const wchar_t* __restrict, const wchar_t* __restrict, va_list); */
float wcstof(const wchar_t* __restrict, wchar_t** __restrict);
long double wcstold(const wchar_t* __restrict, wchar_t** __restrict);
/* TODO: int vwscanf(const wchar_t* __restrict, va_list); */
size_t wcsftime(wchar_t* __restrict, size_t, const wchar_t* __restrict, const struct tm* __restrict);
long long wcstoll(const wchar_t* __restrict, wchar_t** __restrict, int);
unsigned long long wcstoull(const wchar_t* __restrict, wchar_t** __restrict, int);
#endif

/* Functions from X/Open. */
#if __USE_SORTIX || __USE_XOPEN
int wcswidth(const wchar_t*, size_t);
int wcwidth(wchar_t);
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
size_t mbsnrtowcs(wchar_t* __restrict, const char** __restrict, size_t, size_t, mbstate_t* __restrict);
/* TODO: FILE* open_wmemstream(wchar_t**, size_t*); */
wchar_t* wcpcpy(wchar_t* __restrict, const wchar_t* __restrict);
wchar_t* wcpncpy(wchar_t* __restrict, const wchar_t* __restrict, size_t);
int wcscasecmp(const wchar_t*, const wchar_t*);
/* TODO: int wcscasecmp_l(const wchar_t*, const wchar_t*, locale_t); */
/* TODO: int wcscoll_l(const wchar_t*, const wchar_t*, locale_t); */
wchar_t* wcsdup(const wchar_t*);
int wcsncasecmp(const wchar_t*, const wchar_t *, size_t);
/* TODO: int wcsncasecmp_l(const wchar_t*, const wchar_t *, size_t, locale_t); */
size_t wcsnlen(const wchar_t*, size_t);
size_t wcsnrtombs(char* __restrict, const wchar_t** __restrict, size_t, size_t, mbstate_t* __restrict);
/* TODO: size_t wcsxfrm_l(wchar_t* __restrict, const wchar_t* __restrict, size_t, locale_t); */
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
/* TODO: getwc_unlocked */
/* TODO: getwchar_unlocked */
/* TODO: fgetwc_unlocked */
/* TODO: fputwc_unlocked */
/* TODO: putwc_unlocked */
/* TODO: putwchar_unlocked */
/* TODO: fgetws_unlocked */
/* TODO: fputws_unlocked */
wchar_t* wcschrnul(const wchar_t*, wchar_t);
/* TODO: wcsftime_l */
size_t wcslcat(wchar_t* __restrict, const wchar_t* __restrict, size_t);
size_t wcslcpy(wchar_t* __restrict, const wchar_t* __restrict, size_t);
/* TODO: wchar_t* wmempcpy(wchar_t* __restrict, const wchar_t* __restrict, size_t); */
/* TODO: wcstod_l? */
/* TODO: wcstof_l? */
/* TODO: wcstof_ld? */
/* TODO: wcstol_l? */
/* TODO: wcstoll_l? */
/* TODO: wcstoul_l? */
/* TODO: wcstoull_l */
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
