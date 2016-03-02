/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/wait.h
 * Declarations for waiting for the events of children.
 */

#ifndef INCLUDE_SORTIX_WAIT_H
#define INCLUDE_SORTIX_WAIT_H

#include <sys/cdefs.h>

#include <sortix/__/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WCONTINUED (1<<0)
#define WNOHANG (1<<1)
#define WUNTRACED (1<<2)

#define WNATURE_EXITED __WNATURE_EXITED
#define WNATURE_SIGNALED __WNATURE_SIGNALED
#define WNATURE_STOPPED __WNATURE_STOPPED
#define WNATURE_CONTINUED __WNATURE_CONTINUED

#define WEXITSTATUS(status) __WEXITSTATUS(status)
#define WTERMSIG(status) __WTERMSIG(status)
#define WNATURE(status) __WNATURE(status)

#define WIFEXITED(status) __WIFEXITED(status)
#define WIFSIGNALED(status) __WIFSIGNALED(status)
#define WIFSTOPPED(status) __WIFSTOPPED(status)
#define WIFCONTINUED(status) __WIFCONTINUED(status)

#define WSTOPSIG(status) __WSTOPSIG(status)

#define WCONSTRUCT(nature, exitcode, signal) \
        __WCONSTRUCT(nature, exitcode, signal)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
