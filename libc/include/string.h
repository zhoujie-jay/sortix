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

	string.h
	String operations.

*******************************************************************************/

#ifndef	_STRING_H
#define	_STRING_H 1

#include <features.h>
#include <strings.h>

__BEGIN_DECLS

@include(NULL.h)
@include(size_t.h)
@include(locale_t.h)

void* memchr(const void*, int, size_t);
int memcmp(const void*, const void*, size_t);
void* memcpy(void* restrict, const void* restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
char* stpcpy(char* restrict, const char* restrict);
char* strcat(char* restrict, const char* restrict);
char* strchr(const char*, int);
int strcmp(const char*, const char*);
int strcoll(const char*, const char*);
size_t strcspn(const char*, const char*);
char* strcpy(char* restrict, const char* restrict);
char* strdup(const char*);
size_t strlen(const char*);
char* strncat(char* restrict, const char* restrict, size_t);
int strncmp(const char*, const char*, size_t);
char* strncpy(char* restrict, const char* restrict, size_t);
size_t strnlen(const char*, size_t);
char* strpbrk(const char*, const char*);
char* strrchr(const char*, int);
size_t strspn(const char*, const char*);
char* strstr(const char*, const char*);
char* strtok(char* restrict, const char* restrict);
char* strtok_r(char* restrict, const char* restrict, char** restrict);

/* TODO: These are not implemented in sortix libc yet. */
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
void* memccpy(void* restrict, const void* restrict, int, size_t);
char* stpncpy(char* restrict, const char* restrict, size_t);
int strcoll_l(const char*, const char*, locale_t);
char* strerror_l(int, locale_t);
int strerror_r(int, char*, size_t);
char* strndup(const char*, size_t);
char* strsignal(int);
size_t strxfrm(char* restrict, const char* restrict, size_t);
size_t strxfrm_l(char* restrict, const char* restrict, size_t, locale_t);
#endif

#if defined(_SORTIX_SOURCE) || defined(_GNU_SOURCE)
char* strchrnul(const char* str, int c);
#endif

#if defined(_SORTIX_SOURCE)
const char* sortix_strerror(int errnum);
#endif
#if defined(_SOURCE_SOURCE) && __SORTIX_STDLIB_REDIRECTS
const char* strerror(int errnum) asm ("sortix_getenv");
#else
char* strerror(int errnum);
#endif

__END_DECLS

#endif
