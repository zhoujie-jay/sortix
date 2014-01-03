/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

#ifndef INCLUDE_DIRENT_H
#define INCLUDE_DIRENT_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __ino_t_defined
#define __ino_t_defined
typedef __ino_t ino_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __DIR_defined
#define __DIR_defined
typedef struct DIR DIR;
#endif

struct dirent
{
	ino_t d_ino;
	size_t d_reclen;
	char d_name[0];
};

#undef  _DIRENT_HAVE_D_NAMLEN
#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_OFF
#undef  _DIRENT_HAVE_D_TYPE

#define _D_EXACT_NAMLEN(d) ((d)->d_reclen - __builtin_offsetof(struct dirent, d_name) - 1)
#define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN(d) + 1)

int alphasort(const struct dirent**, const struct dirent**);
int closedir(DIR* dir);
int dirfd(DIR* dir);
DIR* fdopendir(int fd);
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dir);
/* TODO: readdir_r */
void rewinddir(DIR* dir);
int scandir(const char*, struct dirent***, int (*)(const struct dirent*),
            int (*)(const struct dirent**, const struct dirent**));
/* TODO: seekdir */
/* TODO: telldir */
int versionsort(const struct dirent**, const struct dirent**);

#if defined(_SORTIX_SOURCE)
void dregister(DIR* dir);
void dunregister(DIR* dir);
DIR* dnewdir(void);
int dcloseall(void);
void dclearerr(DIR* dir);
int derror(DIR* dir);
int deof(DIR* dif);
#endif

#if defined(__is_sortix_libc)
extern DIR* __firstdir;
#endif

__END_DECLS

#endif
