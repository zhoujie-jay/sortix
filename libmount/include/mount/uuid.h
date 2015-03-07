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

    mount/uuid.h
    Universally unique identifier.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_UUID_H
#define INCLUDE_MOUNT_UUID_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#define UUID_STRING_LENGTH 36 /* and of course a nul byte not included here */

#if defined(__cplusplus)
extern "C" {
#endif

bool uuid_validate(const char*);
void uuid_from_string(unsigned char[16], const char*);
void uuid_to_string(const unsigned char[16], char*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
