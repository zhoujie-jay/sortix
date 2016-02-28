/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    stdio/fparsemode.c
    Parses the mode argument of functions like fopen().

*******************************************************************************/

#include <errno.h>
#include <stdio.h>

int fparsemode(const char* mode)
{
	int result;

	switch ( *mode++ )
	{
	case 'r': result = FILE_MODE_READ; break;
	case 'w': result = FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE; break;
	case 'a': result = FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_APPEND; break;
	default: return errno = EINVAL, -1;
	};

	while ( *mode )
	{
		switch ( *mode++ )
		{
		case 'b': result |= FILE_MODE_BINARY; break;
		case 'e': result |= FILE_MODE_CLOEXEC; break;
		case 't': result &= ~FILE_MODE_BINARY; break;
		case 'x': result |= FILE_MODE_EXCL; break;
		case '+': result |= FILE_MODE_READ | FILE_MODE_WRITE; break;
		default: return errno = EINVAL, -1;
		};
	}

	return result;
}
