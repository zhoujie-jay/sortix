/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    sys/utsname/uname.cpp
    Get name of the current system.

*******************************************************************************/

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

extern "C" int uname(struct utsname* name)
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
