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

#ifndef _WCHAR_H
#define _WCHAR_H 1

#include <features.h>

__BEGIN_DECLS

@include(size_t.h)
@include(off_t.h)
@include(FILE.h)
@include(locale_t.h)
@include(va_list.h)
@include(wchar_t.h)
@include(wint_t.h)
@include(WCHAR_MAX.h)
@include(WCHAR_MIN.h)
@include(WEOF.h)
@include(NULL.h)

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

#if __POSIX_OBSOLETE <= 200809L
@include(wctype_t.h)
#endif

struct tm;

size_t wcrtomb(char* restrict, wchar_t, mbstate_t* restrict);
size_t mbrtowc(wchar_t* restrict, const char* restrict, size_t, mbstate_t* restrict);
wchar_t* wcscat(wchar_t* restrict, const wchar_t* restrict);
wchar_t* wcschr(const wchar_t*, wchar_t);
wchar_t* wcschrnul(const wchar_t*, wchar_t);
int wcscmp(const wchar_t*, const wchar_t*);
wchar_t* wcscpy(wchar_t* restrict, const wchar_t* restrict);
size_t wcslen(const wchar_t*);
wchar_t* wcsncat(wchar_t* restrict, const wchar_t* restrict, size_t);
wchar_t* wcsncpy(wchar_t* restrict, const wchar_t* restrict, size_t);
wchar_t* wcsrchr(const wchar_t*, wchar_t);

/* TODO: These are not implemented in sortix libc yet. */
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
double wcstod(const wchar_t* restrict, wchar_t** restrict);
FILE* open_wmemstream(wchar_t** bufp, size_t* sizep);
float wcstof(const wchar_t* restrict, wchar_t** restrict);
int fputws(const wchar_t* restrict, FILE* restrict);
int fwide(FILE*, int);
int fwprintf(FILE* restrict, const wchar_t* restrict, ...);
int fwscanf(FILE* restrict, const wchar_t* restrict, ...);
int mbsinit(const mbstate_t*);
int swprintf(wchar_t* restrict, size_t, const wchar_t* restrict, ...);
int swscanf(const wchar_t* restrict, const wchar_t* restrict, ...);
int vfwprintf(FILE* restrict, const wchar_t* restrict, va_list);
int vfwscanf(FILE* restrict, const wchar_t* restrict, va_list);
int vswprintf(wchar_t* restrict, size_t, const wchar_t* restrict, va_list);
int vswscanf(const wchar_t* restrict, const wchar_t* restrict, va_list);
int vwprintf(const wchar_t* restrict, va_list);
int vwscanf(const wchar_t* restrict, va_list);
int wcscoll(const wchar_t*, const wchar_t*);
int wcsncmp(const wchar_t*, const wchar_t*, size_t);
int wcswidth(const wchar_t*, size_t);
int wctob(wint_t);
int wcwidth(wchar_t);
int wmemcmp(const wchar_t*, const wchar_t*, size_t);
int wprintf(const wchar_t* restrict, ...);
int wscanf(const wchar_t* restrict, ...);
long double wcstold(const wchar_t* restrict, wchar_t** restrict);
long long wcstoll(const wchar_t* restrict, wchar_t** restrict, int);
long wcstol(const wchar_t* restrict, wchar_t** restrict, int);
size_t mbrlen(const char* restrict, size_t, mbstate_t* restrict);
size_t mbsrtowcs(wchar_t* restrict, const char** restrict, size_t, mbstate_t* restrict);
size_t wcscspn(const wchar_t*, const wchar_t*);
size_t wcsftime(wchar_t* restrict, size_t, const wchar_t* restrict, const struct tm* restrict);
size_t wcsrtombs(char* restrict, const wchar_t** restrict, size_t, mbstate_t* restrict);
size_t wcsspn(const wchar_t*, const wchar_t*);
size_t wcsxfrm(wchar_t* restrict, const wchar_t* restrict, size_t);
unsigned long long wcstoull(const wchar_t* restrict, wchar_t** restrict, int);
unsigned long wcstoul(const wchar_t* restrict, wchar_t** restrict, int);
wchar_t* fgetws(wchar_t* restrict, int, FILE* restrict);
wchar_t* wcspbrk(const wchar_t*, const wchar_t*);
wchar_t* wcsstr(const wchar_t* restrict, const wchar_t* restrict);
wchar_t* wcstok(wchar_t* restrict, const wchar_t* restrict, wchar_t** restrict);
wchar_t* wcswcs(const wchar_t*, const wchar_t*);
wchar_t* wmemchr(const wchar_t*, wchar_t, size_t);
wchar_t* wmemcpy(wchar_t* restrict, const wchar_t* restrict, size_t);
wchar_t* wmemmove(wchar_t*, const wchar_t*, size_t);
wchar_t* wmemset(wchar_t*, wchar_t, size_t);
wint_t btowc(int);
wint_t fgetwc(FILE*);
wint_t fputwc(wchar_t, FILE*);
wint_t getwc(FILE*);
wint_t getwchar(void);
wint_t putwchar(wchar_t);
wint_t putwc(wchar_t, FILE*);
wint_t ungetwc(wint_t, FILE*);

#if __POSIX_OBSOLETE <= 200809L
int iswalnum(wint_t);
int iswalpha(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t, wctype_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctype_t wctype(const char *);
#endif
#endif

__END_DECLS

#endif
