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

    string.h
    String operations.

*******************************************************************************/

#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#if __USE_SORTIX || 2008 <= __USE_POSIX
#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif
#endif

void* memchr(const void*, int, size_t);
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
char* strcat(char* __restrict, const char* __restrict);
char* strchr(const char*, int);
int strcmp(const char*, const char*);
int strcoll(const char*, const char*);
char* strcpy(char* __restrict, const char* __restrict);
size_t strcspn(const char*, const char*);
#if __USE_SORTIX && __SORTIX_STDLIB_REDIRECTS
const char* strerror(int errnum) __asm__ ("sortix_strerror");
#else
char* strerror(int errnum);
#endif
size_t strlen(const char*);
char* strncat(char* __restrict, const char* __restrict, size_t);
int strncmp(const char*, const char*, size_t);
char* strncpy(char* __restrict, const char* __restrict, size_t);
char* strpbrk(const char*, const char*);
char* strrchr(const char*, int);
size_t strspn(const char*, const char*);
char* strstr(const char*, const char*);
char* strtok(char* __restrict, const char* __restrict);
size_t strxfrm(char* __restrict, const char* __restrict, size_t);

/* Functions from early POSIX. */
#if __USE_SORTIX || __USE_POSIX
int strcasecmp(const char* a, const char* b);
int strncasecmp(const char* a, const char* b, size_t n);
#endif

/* Functions from early XOPEN. */
#if __USE_SORTIX || __USE_XOPEN
void* memccpy(void* __restrict, const void* __restrict, int, size_t);
#endif

/* Functions from XOPEN 420 gone into POSIX 2008. */
#if __USE_SORTIX || 420 <= __USE_XOPEN || 200809L <= __USE_POSIX
char* strdup(const char*);
#endif

/* Functions from POSIX 2001. */
#if __USE_SORTIX || 200112L <= __USE_POSIX
int ffs(int);
#if __USE_SORTIX && __SORTIX_STDLIB_REDIRECTS
const char* strerror_l(int, locale_t) __asm__ ("sortix_strerror_l");
#else
char* strerror_l(int, locale_t);
#endif
int strerror_r(int, char*, size_t);
char* strtok_r(char* __restrict, const char* __restrict, char** __restrict);
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
char* stpcpy(char* __restrict, const char* __restrict);
char* stpncpy(char* __restrict, const char* __restrict, size_t);
int strcoll_l(const char*, const char*, locale_t);
char* strndup(const char*, size_t);
size_t strnlen(const char*, size_t);
#if __USE_SORTIX && __SORTIX_STDLIB_REDIRECTS
const char* strsignal(int signum) __asm__ ("sortix_strsignal");
#else
char* strsignal(int signum);
#endif
size_t strxfrm_l(char* __restrict, const char* __restrict, size_t, locale_t);
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
int ffsl(long int);
void* memrchr(const void*, int, size_t);
/* TODO: strcasecmp_l */
char* strchrnul(const char* str, int c);
size_t strlcat(char* __restrict, const char* __restrict, size_t);
size_t strlcpy(char* __restrict, const char* __restrict, size_t);
/* TODO: strncasecmp_l */
int strverscmp(const char*, const char*);
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
int ffsll(long long int);
const char* sortix_strerror(int errnum);
const char* sortix_strerror_l(int, locale_t);
const char* sortix_strsignal(int signum);
#endif

#if __USE_SORTIX
/* Duplicate S, returning an identical alloca'd string.  */
#define strdupa(s)                                                            \
  (__extension__                                                              \
    ({                                                                        \
      const char* __old = (s);                                                \
      size_t __len = strlen(__old) + 1;                                       \
      char* __new = (char*) __builtin_alloca(__len);                          \
      (char*) memcpy(__new, __old, __len);                                    \
    }))

/* Return an alloca'd copy of at most N bytes of string.  */
#define strndupa(s, n)                                                        \
  (__extension__                                                              \
    ({                                                                        \
      __const char* __old = (s);                                              \
      size_t __len = strnlen(__old, (n));                                     \
      char* __new = (char*) __builtin_alloca(__len + 1);                      \
      __new[__len] = '\0';                                                    \
      (char*) memcpy(__new, __old, __len);                                    \
    }))
#endif

__END_DECLS

#endif
