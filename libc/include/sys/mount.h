/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sys/mount.h
    Filesystem mount functionality.

*******************************************************************************/

#ifndef INCLUDE_SYS_MOUNT_H
#define INCLUDE_SYS_MOUNT_H

#include <sys/cdefs.h>

#include <sortix/mount.h>

#ifdef __cplusplus
extern "C" {
#endif

int unmount(const char*, int);
int unmountat(int, const char*, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
