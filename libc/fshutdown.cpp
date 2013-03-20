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

    fshutdown.cpp
    Uninstalls the backend from a FILE so another can be reinstalled.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

extern "C" int fshutdown(FILE* fp)
{
	int ret = fflush(fp);
	if ( ret )
	{
		/* TODO: How to report errors here? fclose may need us to return its
		         exact error value, for instance, as with popen/pclose. */;
	}
	ret = fp->close_func ? fp->close_func(fp->user) : ret;
	if ( fp->flags & _FILE_BUFFER_OWNED )
		free(fp->buffer);
	// Resetting the FILE here isn't needed in the case where fclose calls us,
	// but it's nice to zero it out anyway (avoiding state) data, and it's a
	// feature when called by freopen that wishes to reuse the FILE. It also
	// means that the file is always in a consistent state.
	fresetfile(fp);
	return ret;
}
