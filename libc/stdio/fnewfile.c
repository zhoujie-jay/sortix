/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2015.

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

    stdio/fnewfile.c
    Allocates and initializes a simple FILE object ready for construction.

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fnewfile_destroyer(void* user, FILE* fp)
{
	(void) user;
	free(fp);
}

FILE* fnewfile(void)
{
	FILE* fp = (FILE*) malloc(sizeof(FILE) + BUFSIZ);
	if ( !fp )
		return NULL;
	memset(fp, 0, sizeof(FILE));
	fp->buffer = (unsigned char*) (fp + 1);
	fp->free_user = NULL;
	fp->free_func = fnewfile_destroyer;
	fresetfile(fp);
	fregister(fp);
	return fp;
}
