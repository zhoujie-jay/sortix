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

	error.h
	Error reporting functions and utilities.

******************************************************************************/

#ifndef LIBMAXSI_ERROR_H
#define LIBMAXSI_ERROR_H

namespace Maxsi
{
	namespace Error
	{
		// TODO: merge with errno interface.

		const int SUCCESS = 0;
		const int NONE = 1;
		const int DENIED = 2;
		const int NOTFOUND = 3;
		const int NOSUPPORT = 4;
		const int NOTIMPLEMENTED = 5;
		const int PENDING = 6;
		const int BADINPUT = 7;
		const int CORRUPT = 8;
		const int NOMEM = 9;
		const int NOTDIR = 10;
		const int ISDIR = 11;

		const int ENOTBLK = 12;
		const int ENODEV = 13;
		const int EWOULDBLOCK = 14;
		const int EBADF = 15;
		const int EOVERFLOW = 16;
		const int ENOENT = 17;
		const int ENOSPC = 18;
		const int EEXIST = 19;
		const int EROFS = 20;
		const int EINVAL = 21;
		const int ENOTDIR = 22;
		const int ENOMEM = 23;
		const int ERANGE = 24;

		extern int _errornumber;

		inline int Last() { return _errornumber; }
		inline void Set(int error) { _errornumber = error; }
	}
}

#endif

