/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    stdio/tmpfile.c
    Opens an unnamed temporary file.

*******************************************************************************/

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

FILE* tmpfile(void)
{
	// TODO: There is a short interval during which other processes can access
	//       this file. Implement and use O_TMPFILE.
	char path[] = "/tmp/tmp.XXXXXX";
	int fd = mkstemp(path);
	if ( fd < 0 )
		return (FILE*) NULL;
	if ( unlink(path) < 0 )
		return close(fd), (FILE*) NULL;
	FILE* fp = fdopen(fd, "r+");
	if ( !fp )
		return close(fd), (FILE*) NULL;
	return fp;
}
