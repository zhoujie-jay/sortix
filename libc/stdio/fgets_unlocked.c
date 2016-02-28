/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    stdio/fgets_unlocked.c
    Reads a string from a FILE.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

char* fgets_unlocked(char* restrict dest, int size, FILE* restrict fp)
{
	if ( size <= 0 )
		return errno = EINVAL, (char*) NULL;
	int i;
	for ( i = 0; i < size-1; i++ )
	{
		int c = fgetc_unlocked(fp);
		if ( c == EOF )
			break;
		dest[i] = c;
		if ( c == '\n' )
		{
			i++;
			break;
		}
	}
	if ( !i && (ferror_unlocked(fp) || feof_unlocked(fp)) )
		return NULL;
	// TODO: The end-of-file state is lost here if feof(fp) and we are reading
	//       from a terminal that encountered a soft EOF.
	dest[i] = '\0';
	return dest;
}
