/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	stat.h
	Defines the struct stat used for file meta-information and other useful
	macros and values relating to values stored in it.

*******************************************************************************/

#ifndef SORTIX_STAT_H
#define SORTIX_STAT_H

#include <features.h>

__BEGIN_DECLS

struct stat
{
	dev_t st_dev;
	ino_t st_ino;
	mode_t st_mode;
	nlink_t st_nlink;
	uid_t st_uid;
	gid_t st_gid;
	off_t st_size;
	/* TODO: st_atim, st_mtim, st_st_ctim */
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
#define S_IFDIR 0040000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFLNK 0200000 /* not the same as in Linux */
/* TODO: Define the other useful values and implement their features. */

#define S_ISBLK(m) ((m) & S_IFBLK)
#define S_ISDIR(m) ((m) & S_IFDIR)
#define S_ISREG(m) ((m) & S_IFREG)
#define S_ISLNK(m) ((m) & S_IFLNK)
/* TODO: Define the other useful macros and implement their features. */

__END_DECLS

#endif

