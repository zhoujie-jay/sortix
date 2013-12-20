/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/stat.h
    Defines the struct stat used for file meta-information and other useful
    macros and values relating to values stored in it.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_STAT_H
#define INCLUDE_SORTIX_STAT_H

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sortix/timespec.h>

__BEGIN_DECLS

struct stat
{
	dev_t st_dev;
	dev_t st_rdev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	off_t st_size;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	blksize_t st_blksize;
	blkcnt_t st_blocks;
};

#define S_IXOTH 01
#define S_IWOTH 02
#define S_IROTH 03
#define S_IRWXO 07
#define S_IXGRP 010
#define S_IWGRP 020
#define S_IRGRP 040
#define S_IRWXG 070
#define S_IXUSR 0100
#define S_IWUSR 0200
#define S_IRUSR 0400
#define S_IRWXU 0700
#define S_IFMT 0xF000
#define S_IFSOCK 0xC000
#define S_IFLNK 0xA000
#define S_IFREG 0x8000
#define S_IFBLK 0x6000
#define S_IFDIR 0x4000
#define S_IFCHR 0x2000
#define S_IFIFO 0x1000
/* Intentionally not part of Sortix. */
/*#define S_ISUID 0x0800 */
/*#define S_ISGID 0x0400 */
#define S_ISVTX 0x0200
#define S_SETABLE (0777 | 0x0200 | 0x0400 | 0x0800)
#define S_ISSOCK(mode) ((mode & S_IFMT) == S_IFSOCK)
#define S_ISLNK(mode) ((mode & S_IFMT) == S_IFLNK)
#define S_ISREG(mode) ((mode & S_IFMT) == S_IFREG)
#define S_ISBLK(mode) ((mode & S_IFMT) == S_IFBLK)
#define S_ISDIR(mode) ((mode & S_IFMT) == S_IFDIR)
#define S_ISCHR(mode) ((mode & S_IFMT) == S_IFCHR)
#define S_ISFIFO(mode) ((mode & S_IFMT) == S_IFIFO)

__END_DECLS

#endif
