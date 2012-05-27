/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	error.cpp
	Error reporting functions and utilities.

*******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0
#include <libmaxsi/platform.h>
#include <libmaxsi/error.h>
#ifndef SORTIX_KERNEL
#include <libmaxsi/syscall.h>
#include <stdio.h>
#endif

namespace Maxsi {
namespace Error {

extern "C" { int errno = 0; }

#ifndef SORTIX_KERNEL
DEFN_SYSCALL1(int, SysRegisterErrno, SYSCALL_REGISTER_ERRNO, int*);

extern "C" void init_error_functions()
{
	errno = 0;
	SysRegisterErrno(&errno);
}
#endif

extern "C" const char* sortix_strerror(int errnum)
{
	switch ( errnum )
	{
	case ENOTBLK: return "Block device required";
	case ENODEV: return "No such device";
	case EWOULDBLOCK: return "Operation would block";
	case EBADF: return "Bad file descriptor";
	case EOVERFLOW: return "Value too large to be stored in data type";
	case ENOENT: return "No such file or directory";
	case ENOSPC: return "No space left on device";
	case EEXIST: return "File exists";
	case EROFS: return "Read-only file system";
	case EINVAL: return "Invalid argument";
	case ENOTDIR: return "Not a directory";
	case ENOMEM: return "Not enough memory";
	case ERANGE: return "Result too large";
	case EISDIR: return "Is a directory";
	case EPERM: return "Operation not permitted";
	case EIO: return "Input/output error";
	case ENOEXEC: return "Exec format error";
	case EACCES: return "Permission denied";
	case ESRCH: return "No such process";
	case ENOTTY: return "Not a tty";
	case ECHILD: return "No child processes";
	case ENOSYS: return "Function not implemented";
	case ENOTSUP: return "Operation not supported";
	case EBLOCKING: return "Operation is blocking";
	case EINTR: return "Interrupted function call";
	case ENOTEMPTY: return "Directory not empty";
	case EBUSY: return "Device or resource busy";
	case EPIPE: return "Broken pipe";
	case EILSEQ: return "Illegal byte sequence";
	case ELAKE: return "Sit by a lake";
	case EMFILE: return "Too many open files";
	case EAGAIN: return "Resource temporarily unavailable";
	case EEOF: return "End of file";
	case EBOUND: return "Out of bounds";
	case EINIT: return "Not initialized";
	default: return "Unknown error condition";
	}
}

extern "C" char* strerror(int errnum)
{
	return (char*) sortix_strerror(errnum);
}

} // namespace Error
} // namespace Maxsi
