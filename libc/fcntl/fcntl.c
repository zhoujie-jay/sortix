/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
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
 * fcntl/fcntl.c
 * Manipulates a file descriptor.
 */

#include <sys/syscall.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>

DEFN_SYSCALL3(int, sys_fcntl, SYSCALL_FCNTL, int, int, uintptr_t);

int fcntl(int fd, int cmd, ...)
{
	uintptr_t arg;
	va_list ap;
	va_start(ap, cmd);
	switch ( F_DECODE_CMD_TYPE(cmd) )
	{
	case F_TYPE_VOID: arg = 0; break;
	case F_TYPE_INT: arg = (uintptr_t) va_arg(ap, int); break;
	case F_TYPE_LONG: arg = (uintptr_t) va_arg(ap, long); break;
	case F_TYPE_PTR: arg = (uintptr_t) va_arg(ap, void*); break;
	default: return errno = EINVAL, -1;
	}
	va_end(ap);
	return sys_fcntl(fd, cmd, arg);
}
