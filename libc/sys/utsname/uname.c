/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sys/utsname/uname.c
 * Get name of the current system.
 */

#include <sys/kernelinfo.h>
#include <sys/utsname.h>

#include <brand.h>
#include <string.h>
#include <unistd.h>

#if defined(__x86_64__)
static const char* machine = "x86_64";
static const char* processor = "x86_64";
static const char* hwplatform = "x86_64";
#elif defined(__i386__)
#if defined(__i686__)
static const char* machine = "i686";
#elif defined(__i586__)
static const char* machine = "i586";
#elif defined(__i486__)
static const char* machine = "i486";
#else
static const char* machine = "i386";
#endif
static const char* processor = "i386";
static const char* hwplatform = "i386";
#else
static const char* machine = "unknown";
static const char* processor = "unknown";
static const char* hwplatform = "unknown";
#endif
static const char* opsysname = BRAND_OPERATING_SYSTEM_NAME;

int uname(struct utsname* name)
{
	if ( kernelinfo("name", name->sysname, sizeof(name->sysname)) != 0 )
		strlcpy(name->sysname, "unknown", sizeof(name->sysname));
	if ( gethostname(name->nodename, sizeof(name->nodename)) < 0 )
		strlcpy(name->nodename, "unknown", sizeof(name->nodename));
	if ( kernelinfo("version", name->release, sizeof(name->release)) != 0 )
		strlcpy(name->release, "unknown", sizeof(name->release));
	if ( kernelinfo("builddate", name->version, sizeof(name->version)) != 0 )
		strlcpy(name->version, "unknown", sizeof(name->version));
	strlcpy(name->machine, machine, sizeof(name->machine));
	strlcpy(name->processor, processor, sizeof(name->processor));
	strlcpy(name->hwplatform, hwplatform, sizeof(name->hwplatform));
	strlcpy(name->opsysname, opsysname, sizeof(name->opsysname));
	if ( getdomainname(name->domainname, sizeof(name->domainname)) < 0 )
		strlcpy(name->domainname, "unknown", sizeof(name->domainname));
	return 0;
}
