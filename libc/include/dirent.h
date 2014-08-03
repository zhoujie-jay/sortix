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

#include <sortix/__/dirent.h>

#if defined(__is_sortix_libc)
#include <DIR.h>
#endif

__BEGIN_DECLS

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif

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

#define DT_UNKNOWN __DT_UNKNOWN
#define DT_BLK __DT_BLK
#define DT_CHR __DT_CHR
#define DT_DIR __DT_DIR
#define DT_FIFO __DT_FIFO
#define DT_LNK __DT_LNK
#define DT_REG __DT_REG
#define DT_SOCK __DT_SOCK

#define IFTODT(x) __IFTODT(x)
#define DTTOIF(x) __DTTOIF(x)

struct dirent
{
	size_t d_reclen;
	size_t d_namlen;
	ino_t d_ino;
	dev_t d_dev;
	unsigned char d_type;
	__extension__ char d_name[];
};

#define _DIRENT_HAVE_D_RECLEN
#define _DIRENT_HAVE_D_NAMLEN
#undef  _DIRENT_HAVE_D_OFF
#define _DIRENT_HAVE_D_INO
#define _DIRENT_HAVE_D_DEV
#define _DIRENT_HAVE_D_TYPE

#define _D_EXACT_NAMLEN(d) ((d)->d_namlen)
#define _D_ALLOC_NAMLEN(d) (_D_EXACT_NAMLEN(d) + 1)

int alphasort(const struct dirent**, const struct dirent**);
int alphasort_r(const struct dirent**, const struct dirent**, void*);
int closedir(DIR* dir);
int dirfd(DIR* dir);
int dscandir_r(DIR*, struct dirent***,
               int (*)(const struct dirent*, void*),
               void*,
               int (*)(const struct dirent**, const struct dirent**, void*),
               void*);
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
int versionsort_r(const struct dirent**, const struct dirent**, void*);

#if __USE_SORTIX
void dregister(DIR* dir);
void dunregister(DIR* dir);
DIR* dnewdir(void);
void dclearerr(DIR* dir);
int derror(DIR* dir);
int deof(DIR* dif);
#endif

__END_DECLS

#endif
