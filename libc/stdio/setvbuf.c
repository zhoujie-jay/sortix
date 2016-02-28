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

    stdio/setvbuf.c
    Sets up buffering semantics for a FILE.

*******************************************************************************/

#include <stdio.h>

int setvbuf(FILE* fp, char* buf, int mode, size_t size)
{
	flockfile(fp);
	int ret = setvbuf_unlocked(fp, buf, mode, size);
	funlockfile(fp);
	return ret;
}
