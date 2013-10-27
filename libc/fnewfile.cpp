/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    fnewfile.cpp
    Allocates and initializes a simple FILE object ready for construction.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

static void ffreefile(FILE* fp)
{
	free(fp->buffer);
	free(fp);
}

extern "C" FILE* fnewfile(void)
{
	FILE* fp = (FILE*) calloc(sizeof(FILE), 1);
	if ( !fp ) { return NULL; }
	fp->buffersize = BUFSIZ;
	fp->buffer = (char*) malloc(fp->buffersize);
	if ( !fp->buffer ) { free(fp); return NULL; }
	fp->flags = _FILE_AUTO_LOCK;
	fp->free_func = ffreefile;
	fregister(fp);
	return fp;
}
