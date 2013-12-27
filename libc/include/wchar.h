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

    wchar.h
    Wide character support.

*******************************************************************************/

#ifndef INCLUDE_WCHAR_H
#define INCLUDE_WCHAR_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <stdarg.h>

#if defined(__is_sortix_libc)
#include <FILE.h>
#endif

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __FILE_defined
#define __FILE_defined
typedef struct FILE FILE;
#define FILE FILE
#endif

#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif

#ifndef __wchar_t_defined
#define __wchar_t_defined
#define __need_wchar_t
#include <stddef.h>
#endif

#ifndef __wint_t_defined
#define __wint_t_defined
typedef int __wint_t;
typedef __wint_t wint_t;
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX __WCHAR_MAX
#endif

#ifndef WCHAR_MIN
#define WCHAR_MIN __WCHAR_MIN
#endif

#ifndef WEOF
#define WEOF (-1)
#endif

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#ifndef __mbstate_t_defined
	/* Conversion state information. */
	typedef struct
	{
		int __count;
		union
		{
			wint_t __wch;
			char __wchb[4];
		} __value;		/* Value so far. */
	} mbstate_t;
	#define __mbstate_t_defined 1
#endif

struct tm;

size_t mbsrtowcs(wchar_t* __restrict, const char** __restrict, size_t, mbstate_t* __restrict);
size_t wcrtomb(char* __restrict, wchar_t, mbstate_t* __restrict);
size_t mbrlen(const char* __restrict, size_t, mbstate_t* __restrict);
size_t mbrtowc(wchar_t* __restrict, const char* __restrict, size_t, mbstate_t* __restrict);
wchar_t* wcscat(wchar_t* __restrict, const wchar_t* __restrict);
wchar_t* wcschr(const wchar_t*, wchar_t);
wchar_t* wcschrnul(const wchar_t*, wchar_t);
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
wchar_t* wcstok(wchar_t* __restrict, const wchar_t* __restrict, wchar_t** __restrict);
long long wcstoll(const wchar_t* __restrict, wchar_t** __restrict, int);
long wcstol(const wchar_t* __restrict, wchar_t** __restrict, int);
unsigned long long wcstoull(const wchar_t* __restrict, wchar_t** __restrict, int);
unsigned long wcstoul(const wchar_t* __restrict, wchar_t** __restrict, int);
size_t wcsxfrm(wchar_t* __restrict, const wchar_t* __restrict, size_t);
wchar_t* wmemchr(const wchar_t*, wchar_t, size_t);
int wmemcmp(const wchar_t*, const wchar_t*, size_t);
wchar_t* wmemcpy(wchar_t* __restrict, const wchar_t* __restrict, size_t);
wchar_t* wmemmove(wchar_t*, const wchar_t*, size_t);
wchar_t* wmemset(wchar_t*, wchar_t, size_t);

/* TODO: These are not implemented in sortix libc yet. */
#if 0
double wcstod(const wchar_t* __restrict, wchar_t** __restrict);
FILE* open_wmemstream(wchar_t** bufp, size_t* sizep);
float wcstof(const wchar_t* __restrict, wchar_t** __restrict);
int fputws(const wchar_t* __restrict, FILE* __restrict);
int fwide(FILE*, int);
int fwprintf(FILE* __restrict, const wchar_t* __restrict, ...);
int fwscanf(FILE* __restrict, const wchar_t* __restrict, ...);
int mbsinit(const mbstate_t*);
int swprintf(wchar_t* __restrict, size_t, const wchar_t* __restrict, ...);
int swscanf(const wchar_t* __restrict, const wchar_t* __restrict, ...);
int vfwprintf(FILE* __restrict, const wchar_t* __restrict, va_list);
int vfwscanf(FILE* __restrict, const wchar_t* __restrict, va_list);
int vswprintf(wchar_t* __restrict, size_t, const wchar_t* __restrict, va_list);
int vswscanf(const wchar_t* __restrict, const wchar_t* __restrict, va_list);
int vwprintf(const wchar_t* __restrict, va_list);
int vwscanf(const wchar_t* __restrict, va_list);
int wcswidth(const wchar_t*, size_t);
int wctob(wint_t);
int wcwidth(wchar_t);
int wprintf(const wchar_t* __restrict, ...);
int wscanf(const wchar_t* __restrict, ...);
long double wcstold(const wchar_t* __restrict, wchar_t** __restrict);
size_t wcsftime(wchar_t* __restrict, size_t, const wchar_t* __restrict, const struct tm* __restrict);
wchar_t* fgetws(wchar_t* __restrict, int, FILE* __restrict);
wint_t btowc(int);
wint_t fgetwc(FILE*);
wint_t fputwc(wchar_t, FILE*);
wint_t getwc(FILE*);
wint_t getwchar(void);
wint_t putwchar(wchar_t);
wint_t putwc(wchar_t, FILE*);
wint_t ungetwc(wint_t, FILE*);
#endif

__END_DECLS

#endif
