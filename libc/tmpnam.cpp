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

    tmpnam.cpp
    Create path names for temporary files.

*******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

// TODO: Please remove this function and P_tmpdir and L_tmpnam once they have
//       been removed from the C, C++ and POSIX standards.
extern "C" char* tmpnam(char* s)
{
	static char static_string[L_tmpnam+1];
	if ( !s )
		s = static_string;
	static int current_index = 0;
	// TODO: This isn't thread safe!
	snprintf(s, L_tmpnam, "%s/tmpnam.pid%ju.%i", P_tmpdir, (uintmax_t) getpid(),
	         current_index++);
	return s;
}
