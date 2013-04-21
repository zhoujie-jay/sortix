/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    strerror.cpp
    Convert error code to a string.

*******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0
#include <errno.h>
#include <string.h>

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
	case ENODRV: return "No such driver";
	case E2BIG: return "Argument list too long";
	case EFBIG: return "File too large";
	case EXDEV: return "Improper link";
	case ESPIPE: return "Cannot seek on stream";
	case ENAMETOOLONG: return "Filename too long";
	case ELOOP: return "Too many levels of symbolic links";
	case EMLINK: return "Too many links";
	case ENXIO: return "No such device or address";
	case EPROTONOSUPPORT: return "Protocol not supported";
	case EAFNOSUPPORT: return "Address family not supported";
	case ENOTSOCK: return "Not a socket";
	case EADDRINUSE: return "Address already in use";
	case ETIMEDOUT: return "Connection timed out";
	case ECONNREFUSED: return "Connection refused";
	case EDOM: return "Mathematics argument out of domain of function";
	case EINPROGRESS: return "Operation in progress";
	case EALREADY: return "Connection already in progress";
	case ESHUTDOWN: return "Cannot send after transport endpoint shutdown";
	case ECONNABORTED: return "Connection aborted";
	case ECONNRESET: return "Connection reset";
	case EADDRNOTAVAIL: return "Address not available";
	case EISCONN: return "Socket is connected";
	case EFAULT: return "Bad address";
	case EDESTADDRREQ: return "Destinatiohn address required";
	default: return "Unknown error condition";
	}
}

extern "C" char* strerror(int errnum)
{
	return (char*) sortix_strerror(errnum);
}
