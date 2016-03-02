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
 * hostname.cpp
 * System calls for managing the hostname of the current system.
 */

#include <errno.h>
#include <string.h>

#include <sortix/limits.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

static kthread_mutex_t hostname_lock = KTHREAD_MUTEX_INITIALIZER;

static char hostname[HOST_NAME_MAX + 1] = "sortix";
static size_t hostname_length = 6;

int sys_gethostname(char* dst_name, size_t dst_name_size)
{
	if ( dst_name_size == 0 )
		return errno = EINVAL, -1;
	size_t dst_name_length = dst_name_size - 1;
	ScopedLock lock(&hostname_lock);
	if ( dst_name_length < hostname_length )
		errno = ERANGE;
	if ( hostname_length < dst_name_length )
		dst_name_length = hostname_length;
	if ( !CopyToUser(dst_name, hostname, dst_name_length) )
		return -1;
	char nul = '\0';
	if ( !CopyToUser(dst_name + dst_name_length, &nul, 1) )
		return -1;
	return 0;
}

int sys_sethostname(const char* src_name, size_t src_name_size)
{
	char new_hostname[HOST_NAME_MAX + 1];
	if ( sizeof(new_hostname) < src_name_size )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(new_hostname, src_name, src_name_size) )
		return -1;
	size_t new_hostname_length = strnlen(new_hostname, sizeof(new_hostname));
	if ( HOST_NAME_MAX < new_hostname_length )
		return errno = EINVAL, -1;
	new_hostname[new_hostname_length] = '\0';
	ScopedLock lock(&hostname_lock);
	memcpy(hostname, new_hostname, new_hostname_length + 1);
	hostname_length = new_hostname_length;
	return 0;
}

} // namespace Sortix
