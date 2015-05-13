/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sys/resource.h
    Resource limits and operations.

*******************************************************************************/

#ifndef INCLUDE_SYS_RESOURCE_H
#define INCLUDE_SYS_RESOURCE_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __id_t_defined
#define __id_t_defined
typedef __id_t id_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifndef __suseconds_t_defined
#define __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#endif

#ifndef __timeval_defined
#define __timeval_defined
struct timeval
{
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN 1

struct rusage
{
	struct timeval ru_utime;
	struct timeval ru_stime;
};

int getpriority(int, id_t);
int getrlimit(int, struct rlimit*);
int getrusage(int, struct rusage*);
int prlimit(pid_t, int, const struct rlimit*, struct rlimit*);
int setpriority(int, id_t, int);
int setrlimit(int, const struct rlimit*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
