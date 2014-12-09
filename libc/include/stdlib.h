/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if __USE_SORTIX
#include <stdint.h>
#endif

#include <sortix/wait.h>

__BEGIN_DECLS

#define EXIT_SUCCESS (0)
#define EXIT_FAILURE (1)

/* TODO: This random interface is stupid. What should a good value be? */
#define RAND_MAX 32767

#define MB_CUR_MAX 6

typedef struct
{
	int quot;
	int rem;
} div_t;

typedef struct
{
	long quot;
	long rem;
} ldiv_t;

typedef struct
{
	long long quot;
	long long rem;
} lldiv_t;

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

void abort(void) __attribute__ ((__noreturn__));
int abs(int value);
int atexit(void (*function)(void));
double atof(const char* value);
int atoi(const char*);
long atol(const char*);
long long atoll(const char*);
void* bsearch(const void*, const void*, size_t, size_t, int (*)(const void*, const void*));
void* calloc(size_t, size_t);
char* canonicalize_file_name(const char* path);
char* canonicalize_file_name_at(int dirfd, const char* path);
int clearenv(void);
div_t div(int, int);
void exit(int)  __attribute__ ((__noreturn__));
void _Exit(int status)  __attribute__ ((__noreturn__));
void free(void*);
char* getenv(const char*);
long labs(long);
ldiv_t ldiv(long, long);
long long llabs(long long);
lldiv_t lldiv(long long, long long);
void* malloc(size_t);
int mblen(const char*, size_t);
size_t mbstowcs(wchar_t* __restrict, const char* __restrict, size_t);
int mbtowc(wchar_t *__restrict, const char* __restrict, size_t);
int mkstemp(char*);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("mktemp() is racy, use mkstemp()")))
#endif
char* mktemp(char* templ);
int on_exit(void (*function)(int, void*), void* arg);
void qsort(void*, size_t, size_t, int (*)(const void*, const void*));
void qsort_r(void*, size_t, size_t, int (*)(const void*, const void*, void*), void*);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("rand() isn't random, use arc4random()")))
#endif
int rand(void);
void* realloc(void*, size_t);
char* realpath(const char* __restrict, char* __restrict);
int setenv(const char*, const char*, int);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("srand() isn't random, use arc4random()")))
#endif
void srand(unsigned);
double strtod(const char* __restrict, char** __restrict);
float strtof(const char* __restrict, char** __restrict);
long strtol(const char* __restrict, char** __restrict, int);
long double strtold(const char* __restrict, char** __restrict);
unsigned long strtoul(const char* __restrict, char** __restrict, int);
unsigned long long strtoull(const char* __restrict, char** __restrict, int);
long long strtoll(const char* __restrict, char** __restrict, int);
int system(const char*);
int unsetenv(const char*);
size_t wcstombs(char* __restrict, const wchar_t *__restrict, size_t);
int wctomb(char*, wchar_t);

#if defined(__is_sortix_libc)
struct exit_handler
{
	void (*hook)(int, void*);
	void* param;
	struct exit_handler* next;
};
extern struct exit_handler* __exit_handler_stack;
#endif

/* TODO: These are not implemented in sortix libc yet. */
#if 0
long a64l(const char* s);
double drand48(void);
double erand48(unsigned short [3]);
int getsubopt(char**, char* const *, char**);
int grantpt(int);
char* initstate(unsigned, char*, size_t);
long jrand48(unsigned short [3]);
char* l64a(long);
void lcong48(unsigned short [7]);
long lrand48(void);
char* mkdtemp(char*);
long mrand48(void);
long nrand48(unsigned short[3]);
int posix_memalign(void**, size_t, size_t);
int posix_openpt(int);
char* ptsname(int);
long random(void);
int rand_r(unsigned *);
unsigned short *seed48(unsigned short [3]);
void setkey(const char*);
char* setstate(char*);
void srand48(long);
void srandom(unsigned);
int unlockpt(int);
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
uint32_t arc4random(void);
void arc4random_buf(void*, size_t);
uint32_t arc4random_uniform(uint32_t);
void* reallocarray(void*, size_t, size_t);
#endif

__END_DECLS

#endif
