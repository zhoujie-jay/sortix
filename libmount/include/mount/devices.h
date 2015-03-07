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

    mount/devices.h
    Locate block devices.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_DEVICES_H
#define INCLUDE_MOUNT_DEVICES_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

struct harddisk;

#if defined(__cplusplus)
extern "C" {
#endif

bool devices_iterate_path(bool (*)(void*, const char*), void*);
bool devices_iterate_open(bool (*)(void*, struct harddisk*), void*);
bool devices_open_all(struct harddisk***, size_t*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
