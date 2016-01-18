/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2016.

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

    arpa/inet.h
    Definitions for internet operations.

*******************************************************************************/

#ifndef INCLUDE_ARPA_INET_H
#define INCLUDE_ARPA_INET_H

#include <sys/cdefs.h>

#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Functions from POSIX that is considered obsolete due to bad design. */
#if __USE_SORTIX || __USE_POSIX
in_addr_t inet_addr(const char*);
char* inet_ntoa(struct in_addr);
#endif

/* Functions from POSIX. */
#if __USE_SORTIX || __USE_POSIX
const char* inet_ntop(int, const void* __restrict, char* __restrict, socklen_t);
int inet_pton(int, const char* __restrict, void* __restrict);
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
/* TODO: int inet_aton(const char*, struct in_addr*); */
/* TODO: char* inet_neta(in_addr_t, char*, size_t); */
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
