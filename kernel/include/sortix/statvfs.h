/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/statvfs.h
    Virtual filesystem information structure.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_STATVFS_H
#define INCLUDE_SORTIX_STATVFS_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif

#ifndef __fsblkcnt_t_defined
#define __fsblkcnt_t_defined
typedef __fsblkcnt_t fsblkcnt_t;
#endif

#ifndef __fsfilcnt_t_defined
#define __fsfilcnt_t_defined
typedef __fsfilcnt_t fsfilcnt_t;
#endif

struct statvfs
{
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	dev_t f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};

#define ST_RDONLY (1 << 0)
#define ST_NOSUID (1 << 1)

__END_DECLS

#endif
