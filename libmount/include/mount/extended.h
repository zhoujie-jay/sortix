/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of Sortix libmount.

    Sortix libmount is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libmount is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libmount. If not, see <http://www.gnu.org/licenses/>.

    mount/extended.h
    MBR extended partition.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_EXTENDED_H
#define INCLUDE_MOUNT_EXTENDED_H

#include <mount/filesystem.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern const struct filesystem_handler extended_handler;

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
