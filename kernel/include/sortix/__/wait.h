/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * sortix/__/wait.h
 * Declarations for waiting for the events of children.
 */

#ifndef INCLUDE_SORTIX____WAIT_H
#define INCLUDE_SORTIX____WAIT_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __WNATURE_EXITED 0
#define __WNATURE_SIGNALED 1
#define __WNATURE_STOPPED 2
#define __WNATURE_CONTINUED 3

#define __WEXITSTATUS(status) (((status) >> 8) & 0xFF)
#define __WTERMSIG(status) (((status) >> 0) & 0x7F)
#define __WNATURE(status) (((status) >> 16) & 0xFF)

#define __WIFEXITED(status) (__WNATURE(status) == __WNATURE_EXITED)
#define __WIFSIGNALED(status) (__WNATURE(status) == __WNATURE_SIGNALED)
#define __WIFSTOPPED(status) (__WNATURE(status) == __WNATURE_STOPPED)
#define __WIFCONTINUED(status) (__WNATURE(status) == __WNATURE_CONTINUED)

#define __WSTOPSIG(status) __WTERMSIG(status)

#define __WCONSTRUCT(nature, exitcode, signal) \
        (((nature) & 0xFF) << 16 | \
         ((exitcode) & 0xFF) << 8 | \
         ((signal) & 0x7F) << 0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
