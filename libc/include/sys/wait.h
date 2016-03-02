/*
 * Copyright (c) 2011, 2014 Jonas 'Sortie' Termansen.
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
 * sys/wait.h
 * Declarations for waiting.
 */

#ifndef INCLUDE_SYS_WAIT_H
#define INCLUDE_SYS_WAIT_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

pid_t wait(int* stat_loc);
/* TODO: int waitid(idtype_t, id_t, siginfo_t*, int); */
pid_t waitpid(pid_t pid, int *stat_loc, int options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
