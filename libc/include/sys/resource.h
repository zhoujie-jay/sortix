/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sys/resource.h
 * Resource limits and operations.
 */

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
