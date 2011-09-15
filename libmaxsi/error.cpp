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

#include "platform.h"
#include "error.h"
#include "syscall.h"

namespace Maxsi
{
	namespace Error
	{
		DEFN_SYSCALL1(int, SysRegisterErrno, 28, int*);

		extern "C" { int errno = 0; }

		extern "C" void init_error_functions()
		{
			errno = 0;
			SysRegisterErrno(&errno);			
		}

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
				case ENOEXEC: return (char*) "Not executable";
				case EACCESS: return (char*) "Permission denied";
				case ESRCH: return (char*) "No such process";
				default: return (char*) "Unknown error condition";
			}
		}
	}
}

