/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	kernelinfo.cpp
	Lets user-space query information about the kernel.

*******************************************************************************/

#include "platform.h"
#include <libmaxsi/error.h>
#include <libmaxsi/string.h>
#include "syscall.h"
#include "kernelinfo.h"

#ifndef VERSIONSTR
#define VERSIONSTR "unknown"
#endif

using namespace Maxsi;

namespace Sortix {
namespace Info {

const char* KernelInfo(const char* req)
{
	if ( String::Compare(req, "name") == 0 ) { return "Sortix"; }
	if ( String::Compare(req, "version") == 0 ) { return VERSIONSTR; }
	if ( String::Compare(req, "builddate") == 0 ) { return __DATE__; }
	if ( String::Compare(req, "buildtime") == 0 ) { return __TIME__; }
	return NULL;
}

ssize_t SysKernelInfo(const char* req, char* resp, size_t resplen)
{
	const char* str = KernelInfo(req);
	if ( !str ) { Error::Set(EINVAL); return -1; }
	size_t stringlen = String::Length(str);
	if ( resplen < stringlen + 1 )
	{
		Error::Set(ERANGE);
		return (ssize_t) stringlen;
	}
	String::Copy(resp, str);
	return 0;
}

void Init()
{
	Syscall::Register(SYSCALL_KERNELINFO, (void*) SysKernelInfo);
}

} // namespace Info
} // namespace Sortix
