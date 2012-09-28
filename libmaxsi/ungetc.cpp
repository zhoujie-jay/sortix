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

	ungetc.cpp
	Inserts data in front of the current read queue, allowing applications to
	peek the incoming data and pretend they didn't.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>

extern "C" int ungetc(int c, FILE* fp)
{
	if ( fp->numpushedback == _FILE_MAX_PUSHBACK ) { errno = ERANGE; return EOF; }
	unsigned char uc = c;
	fp->pushedback[fp->numpushedback++] = uc;
	return uc;
}
