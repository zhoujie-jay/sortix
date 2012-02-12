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

	sys/readdirents.h
	Allows reading directory entries directly from a file descriptor.

******************************************************************************/

#ifndef	_SYS_READDIRENTS_H
#define	_SYS_READDIRENTS_H 1

#include <features.h>

__BEGIN_DECLS

@include(size_t.h)

// Keep this up to date with <sortix/directory.h>
struct sortix_dirent
{
	struct sortix_dirent* d_next;
	unsigned char d_type;
	size_t d_namelen;
	char d_name[];
};

int readdirents(int fd, struct sortix_dirent* dirent, size_t size);

__END_DECLS

#endif

