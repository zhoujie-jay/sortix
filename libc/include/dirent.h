/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

#ifdef __cplusplus
extern "C" {
#endif

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
typedef struct __DIR DIR;
#endif

#if __USE_SORTIX
#define DT_UNKNOWN __DT_UNKNOWN
#define DT_BLK __DT_BLK
#define DT_CHR __DT_CHR
#define DT_DIR __DT_DIR
#define DT_FIFO __DT_FIFO
#define DT_LNK __DT_LNK
#define DT_REG __DT_REG
#define DT_SOCK __DT_SOCK
#endif

#if __USE_SORTIX
#define IFTODT(x) __IFTODT(x)
#define DTTOIF(x) __DTTOIF(x)
#endif

struct dirent
{
	size_t d_reclen;
	size_t d_namlen;
	ino_t d_ino;
	dev_t d_dev;
	unsigned char d_type;
	__extension__ char d_name[];
};

int closedir(DIR* dir);
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dir);
void rewinddir(DIR* dir);

#if __USE_SORTIX || __USE_XOPEN
/* TODO: seekdir */
/* TODO: telldir */
#endif

/* Functions from POSIX 1995. */
#if 199506L <= __USE_POSIX
/* readdir_r will not be implemented. */
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
int alphasort(const struct dirent**, const struct dirent**);
int dirfd(DIR* dir);
DIR* fdopendir(int fd);
int scandir(const char*, struct dirent***, int (*)(const struct dirent*),
            int (*)(const struct dirent**, const struct dirent**));
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
int versionsort(const struct dirent**, const struct dirent**);
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
int alphasort_r(const struct dirent**, const struct dirent**, void*);
void dclearerr(DIR* dir);
int deof(DIR* dif);
int derror(DIR* dir);
DIR* dnewdir(void);
void dregister(DIR* dir);
int dscandir_r(DIR*, struct dirent***,
               int (*)(const struct dirent*, void*),
               void*,
               int (*)(const struct dirent**, const struct dirent**, void*),
               void*);
void dunregister(DIR* dir);
int versionsort_r(const struct dirent**, const struct dirent**, void*);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
