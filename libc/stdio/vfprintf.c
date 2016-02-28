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

    stdio/vfprintf.c
    Prints a string to a FILE.

*******************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

int vfprintf(FILE* fp, const char* restrict format, va_list list)
{
	flockfile(fp);
	int ret = vfprintf_unlocked(fp, format, list);
	funlockfile(fp);
	return ret;
}
