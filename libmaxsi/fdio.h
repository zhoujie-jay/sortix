/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	fdio.h
	Handles the file descriptor backend for the FILE* API.

******************************************************************************/

#ifndef _FDIO_H
#define _FDIO_H 1

#include <features.h>

__BEGIN_DECLS

int fdio_install(FILE* fp, const char* mode, int fd);
FILE* fdio_newfile(int fd, const char* mode);

__END_DECLS

#endif

