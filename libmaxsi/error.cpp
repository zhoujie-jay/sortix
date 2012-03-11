/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/error.h>
#ifndef SORTIX_KERNEL
#include <libmaxsi/syscall.h>
#include <stdio.h>
#endif

namespace Maxsi
{
	namespace Error
	{
		extern "C" { int errno = 0; }

#ifndef SORTIX_KERNEL
		DEFN_SYSCALL1(int, SysRegisterErrno, SYSCALL_REGISTER_ERRNO, int*);

		extern "C" void init_error_functions()
		{
			errno = 0;
			SysRegisterErrno(&errno);			
		}
#endif

		extern "C" char* strerror(int code)
		{
			switch ( code )
			{
				case ENOTBLK: return (char*) "Block device required";
				case ENODEV: return (char*) "No such device";
				case EWOULDBLOCK: return (char*) "Operation would block";
				case EBADF: return (char*) "Bad file descriptor";
				case EOVERFLOW: return (char*) "Value too large to be stored in data type";
				case ENOENT: return (char*) "No such file or directory";
				case ENOSPC: return (char*) "No space left on device";
				case EEXIST: return (char*) "File exists";
				case EROFS: return (char*) "Read-only file system";
				case EINVAL: return (char*) "Invalid argument";
				case ENOTDIR: return (char*) "Not a directory";
				case ENOMEM: return (char*) "Not enough space";
				case ERANGE: return (char*) "Result too large";
				case EISDIR: return (char*) "Is a directory";
				case EPERM: return (char*) "Operation not permitted";
				case EIO: return (char*) "Input/output error";
				case ENOEXEC: return (char*) "Exec format error";
				case EACCESS: return (char*) "Permission denied";
				case ESRCH: return (char*) "No such process";
				case ENOTTY: return (char*) "Not a tty";
				case ECHILD: return (char*) "No child processes";
				case ENOSYS: return (char*) "Function not implemented";
				case ENOTSUP: return (char*) "Operation not supported";
				case EBLOCKING: return (char*) "Operation is blocking";
				case EINTR: return (char*) "Interrupted function call";
				case ENOTEMPTY: return (char*) "Directory not empty";
				case EBUSY: return (char*) "Device or resource busy";
				case EPIPE: return (char*) "Broken pipe";
				case ELAKE: return (char*) "Sit by a lake";
				default: return (char*) "Unknown error condition";
			}
		}
	}
}

