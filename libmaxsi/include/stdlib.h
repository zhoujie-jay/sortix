/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	stdlib.h
	Standard library definitions.

******************************************************************************/

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

void abort(void);
int atoi(const char*);
long atol(const char*);
long long atoll(const char*);
void* calloc(size_t, size_t);
void exit(int);
void _Exit(int status);
void free(void*);
char* getenv(const char*);
void* malloc(size_t);
#if !defined(_SORTIX_SOURCE)
char* mktemp(char* templ);
#endif
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
int rand(void);
void* realloc(void*, size_t);
long strtol(const char* restrict, char** restrict, int);
unsigned long strtoul(const char* restrict, char** restrict, int);
unsigned long long strtoull(const char* restrict, char** restrict, int);
long long strtoll(const char* restrict, char** restrict, int);

/* TODO: These are not implemented in libmaxsi/sortix yet. */
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
long a64l(const char* s);
int abs(int value);
int atexit(void (*function)(void));
double atof(const char* value);
void* bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));
div_t div(int, int);
double drand48(void);
double erand48(unsigned short [3]);
int getsubopt(char**, char* const *, char**);
int grantpt(int);
char* initstate(unsigned, char*, size_t);
long jrand48(unsigned short [3]);
char* l64a(long);
long labs(long);
void lcong48(unsigned short [7]);
ldiv_t ldiv(long, long);
long long llabs(long long);
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
int putenv(char*);
long random(void);
char* realpath(const char* restrict, char* restrict);
unsigned short *seed48(unsigned short [3]);
int setenv(const char*, const char*, int);
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
int unsetenv(const char*);
size_t wcstombs(char* restrict, const wchar_t *restrict, size_t);
int wctomb(char*, wchar_t);

#if __POSIX_OBSOLETE <= 200801
int rand_r(unsigned *);
#endif

#endif

__END_DECLS

#endif
