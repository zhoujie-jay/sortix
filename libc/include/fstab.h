/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    fstab.h
    Filesystem table.

*******************************************************************************/

#ifndef INCLUDE_FSTAB_H
#define INCLUDE_FSTAB_H

#include <sys/cdefs.h>

#ifndef __FILE_defined
#define __FILE_defined
typedef struct __FILE FILE;
#endif

struct fstab
{
	char* fs_spec;
	char* fs_file;
	char* fs_vfstype;
	char* fs_mntops;
	char* fs_type;
	int fs_freq;
	int fs_passno;
};

#define	FSTAB_RO "ro"
#define	FSTAB_RQ "rq"
#define	FSTAB_RW "rw"
#define	FSTAB_SW "sw"
#define	FSTAB_XX "xx"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__is_sortix_libc)
extern FILE* __fstab_file;
#endif

void endfsent(void);
struct fstab* getfsent(void);
struct fstab* getfsfile(const char*);
struct fstab* getfsspec(const char*);
int scanfsent(char*, struct fstab*);
int setfsent(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
