/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011.

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

	dirent.h
	Format of directory entries.

*******************************************************************************/

#ifndef _DIRENT_H
#define _DIRENT_H 1

#include <features.h>

__BEGIN_DECLS

@include(size_t.h)
@include(DIR.h)

struct dirent
{
	size_t d_reclen;
	char d_name[0];
};

int closedir(DIR* dir);
int dirfd(DIR* dir);
DIR* fdopendir(int fd);
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dir);
void rewinddir(DIR* dir);

#if defined(_SORTIX_SOURCE)
void dregister(DIR* dir);
void dunregister(DIR* dir);
DIR* dnewdir(void);
int dcloseall(void);
void dclearerr(DIR* dir);
int derror(DIR* dir);
int deof(DIR* dif);
#endif

__END_DECLS

#endif
