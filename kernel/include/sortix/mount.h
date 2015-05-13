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

    sortix/mount.h
    Constants related to mounting and binding.

*******************************************************************************/

#ifndef SORTIX_INCLUDE_MOUNT_H
#define SORTIX_INCLUDE_MOUNT_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNMOUNT_FORCE (1 << 0)
#define UNMOUNT_DETACH (1 << 1)
#define UNMOUNT_NOFOLLOW (1 << 2)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
