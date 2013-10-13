/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sortix/dirent.h
    Format of directory entries.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_DIRENT_H
#define INCLUDE_SORTIX_DIRENT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define DT_UNKNOWN 0
#define DT_BLK 1
#define DT_CHR 2
#define DT_DIR 3
#define DT_FIFO 4
#define DT_LNK 5
#define DT_REG 6
#define DT_SOCK 7

struct kernel_dirent
{
	size_t d_reclen;
	size_t d_off;
	size_t d_namelen;
	ino_t d_ino;
	dev_t d_dev;
	unsigned char d_type;
	char d_name[];
};

static inline struct kernel_dirent* kernel_dirent_next(struct kernel_dirent* ent)
{
	if ( !ent->d_off )
		return NULL;
	return (struct kernel_dirent*) ((uint8_t*) ent + ent->d_off);
}

__END_DECLS

#endif
