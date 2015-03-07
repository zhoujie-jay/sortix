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

    mount/error.h
    Error enumerations.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_ERROR_H
#define INCLUDE_MOUNT_ERROR_H

enum filesystem_error
{
	FILESYSTEM_ERROR_NONE,
	FILESYSTEM_ERROR_ABSENT,
	FILESYSTEM_ERROR_UNRECOGNIZED,
	FILESYSTEM_ERROR_ERRNO,
};

enum partition_error
{
	PARTITION_ERROR_NONE,
	PARTITION_ERROR_ABSENT,
	PARTITION_ERROR_UNRECOGNIZED,
	PARTITION_ERROR_ERRNO,
	PARTITION_ERROR_INVALID,
	PARTITION_ERROR_HEADER_TOO_LARGE,
	PARTITION_ERROR_CHECKSUM,
	PARTITION_ERROR_OVERLAP,
	PARTITION_ERROR_END_BEFORE_START,
	PARTITION_ERROR_BEFORE_USABLE,
	PARTITION_ERROR_BEYOND_DEVICE,
	PARTITION_ERROR_BEYOND_EXTENDED,
	PARTITION_ERROR_BEYOND_USABLE,
	PARTITION_ERROR_TOO_EXTENDED,
	PARTITION_ERROR_BAD_EXTENDED,
	PARTITION_ERROR_NONLINEAR_EXTENDED,
};

#endif
