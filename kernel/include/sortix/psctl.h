/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * sortix/psctl.h
 * Process control interface.
 */

#ifndef INCLUDE_SORTIX_PSCTL_H
#define INCLUDE_SORTIX_PSCTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/tmns.h>

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

#define __PSCTL(s, v) (sizeof(struct s) << 16 | (v))

#define PSCTL_PREV_PID __PSCTL(psctl_prev_pid, 1)
struct psctl_prev_pid
{
	pid_t prev_pid;
};

#define PSCTL_NEXT_PID __PSCTL(psctl_next_pid, 2)
struct psctl_next_pid
{
	pid_t next_pid;
};

#define PSCTL_STAT __PSCTL(psctl_stat, 3)
struct psctl_stat
{
	pid_t pid;
	pid_t ppid;
	pid_t ppid_prev;
	pid_t ppid_next;
	pid_t ppid_first;
	pid_t pgid;
	pid_t pgid_prev;
	pid_t pgid_next;
	pid_t pgid_first;
	pid_t sid;
	pid_t sid_prev;
	pid_t sid_next;
	pid_t sid_first;
	pid_t init;
	pid_t init_prev;
	pid_t init_next;
	pid_t init_first;
	uid_t uid;
	uid_t euid;
	gid_t gid;
	gid_t egid;
	int status;
	int nice;
	struct tmns tmns;
};

#define PSCTL_PROGRAM_PATH __PSCTL(psctl_program_path, 4)
struct psctl_program_path
{
	char* buffer;
	size_t size;
};

#endif
