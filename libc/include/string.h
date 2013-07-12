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

    string.h
    String operations.

*******************************************************************************/

#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H

#include <features.h>
#include <strings.h>

__BEGIN_DECLS

@include(NULL.h)
@include(size_t.h)
@include(locale_t.h)

void* memccpy(void* __restrict, const void* __restrict, int, size_t);
void* memchr(const void*, int, size_t);
int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
char* stpcpy(char* __restrict, const char* __restrict);
char* stpncpy(char* __restrict, const char* __restrict, size_t);
int strcasecmp(const char* a, const char* b);
char* strcat(char* __restrict, const char* __restrict);
char* strchr(const char*, int);
int strcmp(const char*, const char*);
int strcoll(const char*, const char*);
int strcoll_l(const char*, const char*, locale_t);
size_t strcspn(const char*, const char*);
char* strcpy(char* __restrict, const char* __restrict);
char* strdup(const char*);
int strerror_r(int, char*, size_t);
size_t strlcat(char* __restrict, const char* __restrict, size_t);
size_t strlcpy(char* __restrict, const char* __restrict, size_t);
size_t strlen(const char*);
int strncasecmp(const char* a, const char* b, size_t n);
char* strncat(char* __restrict, const char* __restrict, size_t);
int strncmp(const char*, const char*, size_t);
char* strncpy(char* __restrict, const char* __restrict, size_t);
char* strndup(const char*, size_t);
size_t strnlen(const char*, size_t);
char* strpbrk(const char*, const char*);
char* strrchr(const char*, int);
size_t strspn(const char*, const char*);
char* strstr(const char*, const char*);
char* strtok(char* __restrict, const char* __restrict);
char* strtok_r(char* __restrict, const char* __restrict, char** __restrict);
int strverscmp(const char*, const char*);
size_t strxfrm(char* __restrict, const char* __restrict, size_t);
size_t strxfrm_l(char* __restrict, const char* __restrict, size_t, locale_t);

#if defined(_SORTIX_SOURCE) || defined(_GNU_SOURCE)
char* strchrnul(const char* str, int c);
#endif

#if defined(_SORTIX_SOURCE)
const char* sortix_strerror(int errnum);
const char* sortix_strerror_l(int, locale_t);
const char* sortix_strsignal(int signum);
#endif
#if defined(_SOURCE_SOURCE) && __SORTIX_STDLIB_REDIRECTS
const char* strerror(int errnum) asm ("sortix_strerror");
const char* strerror_l(int, locale_t) asm ("sortix_strerror_l");
const char* strsignal(int signum) asm ("sortix_strsignal");
#else
char* strerror(int errnum);
char* strerror_l(int, locale_t);
char* strsignal(int signum);
#endif

__END_DECLS

#endif
