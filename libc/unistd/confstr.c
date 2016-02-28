/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    unistd/confstr.c
    Get configurable string variables.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

size_t confstr(int name, char* buf, size_t len)
{
	switch ( name )
	{
		(void) buf;
		(void) len;
	default:
		fprintf(stderr, "%s:%u warning: %s(%i, ...) is unsupported\n",
		        __FILE__, __LINE__, __func__, name);
		return errno = EINVAL, 0;
	}
}
