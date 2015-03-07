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

    filesystem.c
    Filesystem abstraction.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mount/biosboot.h>
#include <mount/blockdevice.h>
#include <mount/ext2.h>
#include <mount/extended.h>
#include <mount/filesystem.h>
#include <mount/partition.h>

const char* filesystem_error_string(enum filesystem_error error)
{
	switch ( error )
	{
	case FILESYSTEM_ERROR_NONE:
		break;
	case FILESYSTEM_ERROR_ABSENT:
		return "No filesystem found";
	case FILESYSTEM_ERROR_UNRECOGNIZED:
		return "Unrecognized filesystem type";
	case FILESYSTEM_ERROR_ERRNO:
		return (const char*) strerror(errno);
	}
	return "Unknown error condition";
}

static const struct filesystem_handler* filesystem_handlers[] =
{
	&biosboot_handler,
	&extended_handler,
	&ext2_handler,
	NULL,
};

void filesystem_release(struct filesystem* fs)
{
	if ( !fs || !fs->handler )
		return;
	fs->handler->release(fs);
}

enum filesystem_error
blockdevice_inspect_filesystem(struct filesystem** fs_ptr,
                               struct blockdevice* bdev)
{
	*fs_ptr = NULL;
	size_t leading_size = 65536;
	for ( size_t i = 0; filesystem_handlers[i]; i++ )
	{
		size_t amount = filesystem_handlers[i]->probe_amount(bdev);
		if ( leading_size < amount )
			leading_size = amount;
	}
	unsigned char* leading = (unsigned char*) malloc(leading_size);
	if ( !leading )
		return *fs_ptr = NULL, FILESYSTEM_ERROR_ERRNO;
	size_t amount = blockdevice_preadall(bdev, leading, leading_size, 0);
	if ( amount < leading_size && errno != EEOF )
		return free(leading), *fs_ptr = NULL, FILESYSTEM_ERROR_ERRNO;
	for ( size_t i = 0; filesystem_handlers[i]; i++ )
	{
		if ( !filesystem_handlers[i]->probe(bdev, leading, amount) )
			continue;
		free(leading);
		return filesystem_handlers[i]->inspect(fs_ptr, bdev);
	}
	for ( size_t i = 0; i < amount; i++ )
		if ( !leading[i] )
			return free(leading), FILESYSTEM_ERROR_UNRECOGNIZED;
	return free(leading), FILESYSTEM_ERROR_ABSENT;
}
