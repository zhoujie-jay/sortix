/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	kernelinfo.cpp
	Queries information about the kernel.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <sys/kernelinfo.h>

namespace Maxsi {

DEFN_SYSCALL3(ssize_t, SysKernelInfo, SYSCALL_KERNELINFO, const char*, char*, size_t);

extern "C" ssize_t kernelinfo(const char* req, char* resp, size_t resplen)
{
	return SysKernelInfo(req, resp, resplen);
}

} // namespace Maxsi

