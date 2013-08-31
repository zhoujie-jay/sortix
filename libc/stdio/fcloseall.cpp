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

    stdio/fcloseall.cpp
    Closes and flushes all open registered files.

*******************************************************************************/

#include <stdio.h>

extern "C" int fcloseall(void)
{
	int result = 0;
	// We do not lock __first_file_lock here because this function is called on
	// process termination and only one thread can call exit(3).
	while ( __first_file )
		result |= fclose(__first_file);
	return result ? EOF : 0;
}
