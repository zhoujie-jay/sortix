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

    stdio/tmpfile.cpp
    Opens an unnamed temporary file.

*******************************************************************************/

#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

extern "C" FILE* tmpfile()
{
	char name[5 + sizeof(pid_t) * 3];
	snprintf(name, sizeof(name), "/tmp/%ju", (uintmax_t) getpid());
	FILE* ret = fopen(name, "w+");
	if ( !ret )
		return NULL;
	unlink(name);
	return ret;
}
