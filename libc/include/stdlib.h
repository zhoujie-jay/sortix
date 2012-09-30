/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	stdlib.h
	Standard library definitions.

*******************************************************************************/

#ifndef	_STDLIB_H
#define	_STDLIB_H 1

#include <features.h>

__BEGIN_DECLS

#define EXIT_SUCCESS (0)
#define EXIT_FAILURE (1)

/* TODO: This random interface is stupid. What should a good value be? */
#define RAND_MAX 32767

/* TODO: This is just a value. It's not a compile time constant! */
#define MB_CUR_MAX 16

/* TODO: div_t, ldiv_t, and lldiv_t is missing here */
typedef int div_t, ldiv_t, lldiv_t;

@include(NULL.h)
@include(size_t.h)
@include(wchar_t.h)

/* TODO: WEXITSTATUS, WIFEXITED, WIFSIGNALED, WIFSTOPPED, WNOHANG, WSTOPSIG, WTERMSIG, WUNTRACED is missing here */

void abort(void) __attribute__ ((noreturn));
int abs(int value);
int atexit(void (*function)(void));
int atoi(const char*);
long atol(const char*);
long long atoll(const char*);
void* bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));
void* calloc(size_t, size_t);
void exit(int)  __attribute__ ((noreturn));
void _Exit(int status)  __attribute__ ((noreturn));
void free(void*);
long labs(long);
long long llabs(long long);
void* malloc(size_t);
#if !defined(_SORTIX_SOURCE)
char* mktemp(char* templ);
#endif
int on_exit(void (*function)(int, void*), void* arg);
int putenv(char*);
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
int rand(void);
void* realloc(void*, size_t);
int setenv(const char*, const char*, int);
long strtol(const char* restrict, char** restrict, int);
unsigned long strtoul(const char* restrict, char** restrict, int);
unsigned long long strtoull(const char* restrict, char** restrict, int);
long long strtoll(const char* restrict, char** restrict, int);
int unsetenv(const char*);

#if defined(_SORTIX_SOURCE) || defined(_WANT_SORTIX_ENV)
const char* const* getenviron(void);
size_t envlength(void);
const char* getenvindexed(size_t index);
const char* sortix_getenv(const char* name);
#endif
#if (defined(_SOURCE_SOURCE) && __SORTIX_STDLIB_REDIRECTS) || \
    defined(_WANT_SORTIX_ENV)
const char* getenv(const char* name) asm ("sortix_getenv");
#else
char* getenv(const char*);
#endif
#if defined(_SORTIX_SOURCE) || defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) \
    || defined(_GNU_SOURCE) || defined(_BSD_SOURCE)
int clearenv(void);
#endif

/* TODO: These are not implemented in sortix libc yet. */
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
long a64l(const char* s);
double atof(const char* value);
div_t div(int, int);
double drand48(void);
double erand48(unsigned short [3]);
int getsubopt(char**, char* const *, char**);
int grantpt(int);
char* initstate(unsigned, char*, size_t);
long jrand48(unsigned short [3]);
char* l64a(long);
void lcong48(unsigned short [7]);
ldiv_t ldiv(long, long);
lldiv_t lldiv(long long, long long);
long lrand48(void);
int mblen(const char*, size_t);
size_t mbstowcs(wchar_t *restrict, const char* restrict, size_t);
int mbtowc(wchar_t *restrict, const char* restrict, size_t);
char* mkdtemp(char*);
int mkstemp(char*);
long mrand48(void);
long nrand48(unsigned short[3]);
int posix_memalign(void**, size_t, size_t);
int posix_openpt(int);
char* ptsname(int);
long random(void);
char* realpath(const char* restrict, char* restrict);
unsigned short *seed48(unsigned short [3]);
void setkey(const char*);
char* setstate(char*);
void srand(unsigned);
void srand48(long);
void srandom(unsigned);
double strtod(const char* restrict, char** restrict);
float strtof(const char* restrict, char** restrict);
long double strtold(const char* restrict, char** restrict);
int system(const char*);
int unlockpt(int);
size_t wcstombs(char* restrict, const wchar_t *restrict, size_t);
int wctomb(char*, wchar_t);

#if __POSIX_OBSOLETE <= 200801
int rand_r(unsigned *);
#endif

#endif

__END_DECLS

#endif