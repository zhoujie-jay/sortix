/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014, 2015.

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

#include <sys/__/types.h>

#include <stdint.h>

#include <sortix/__/dirent.h>
#include <sortix/__/dt.h>

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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
