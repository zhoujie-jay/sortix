/*
 * Copyright (c) 2012, 2015 Jonas 'Sortie' Termansen.
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
 * kernelinfo.cpp
 * Lets user-space query information about the kernel.
 */

#include <brand.h>
#include <errno.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/syscall.h>

#ifndef VERSIONSTR
#define VERSIONSTR "unknown"
#endif

namespace Sortix {

static const char* KernelInfo(const char* req)
{
	if ( strcmp(req, "name") == 0 ) { return BRAND_KERNEL_NAME; }
	if ( strcmp(req, "version") == 0 ) { return VERSIONSTR; }
	if ( strcmp(req, "builddate") == 0 ) { return __DATE__; }
	if ( strcmp(req, "buildtime") == 0 ) { return __TIME__; }
#if defined(__i386__) || defined(__x86_64__)
	if ( strcmp(req, "firmware") == 0 ) { return "bios"; }
#else
	#warning "Name your system firmware here"
#endif
	return NULL;
}

ssize_t sys_kernelinfo(const char* req, char* resp, size_t resplen)
{
	const char* str = KernelInfo(req);
	if ( !str )
		return errno = EINVAL, -1;
	size_t stringlen = strlen(str);
	if ( resplen < stringlen + 1 )
		return errno = ERANGE, (ssize_t) stringlen;
	if ( !CopyToUser(resp, str, sizeof(char) * (stringlen + 1)) )
		return -1;
	return 0;
}

} // namespace Sortix
