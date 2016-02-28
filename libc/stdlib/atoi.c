/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    stdlib/atoi.c
    Converts integers represented as strings to binary representation.

*******************************************************************************/

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int atoi(const char* str)
{
	long long_result = strtol(str, (char**) NULL, 10);
	if ( long_result < INT_MIN )
		return errno = ERANGE, INT_MIN;
	if ( INT_MAX < long_result )
		return errno = ERANGE, INT_MAX;
	return (int) long_result;
}
