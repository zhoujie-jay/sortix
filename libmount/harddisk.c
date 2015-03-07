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

    harddisk.c
    Harddisk abstraction.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <mount/harddisk.h>

#include "util.h"

static char* atcgetblob(int fd, const char* name, size_t* size_ptr)
{
	ssize_t size = tcgetblob(fd, name, NULL, 0);
	if ( size < 0 )
		return NULL;
	char* result = (char*) malloc((size_t) size + 1);
	if ( !result )
		return NULL;
	ssize_t second_size = tcgetblob(fd, name, result, (size_t) size);
	if ( second_size != size )
		return free(result), (char*) NULL;
	result[(size_t) size] = '\0';
	if ( size_ptr )
		*size_ptr = (size_t) size;
	return result;
}

struct harddisk* harddisk_openat(int dirfd, const char* relpath, int rdwr)
{
	struct harddisk* hd = CALLOC_TYPE(struct harddisk);
	if ( !hd )
		return (struct harddisk*) NULL;
	memset(&hd->bdev, 0, sizeof(hd->bdev));
	hd->bdev.hd = hd;
	if ( (hd->fd = openat(dirfd, relpath, rdwr)) < 0 )
		return harddisk_close(hd), (struct harddisk*) NULL;
	if ( !(hd->path = canonicalize_file_name_at(dirfd, relpath)) )
		return harddisk_close(hd), (struct harddisk*) NULL;
	if ( fstat(hd->fd, &hd->st) < 0 )
		return harddisk_close(hd), (struct harddisk*) NULL;
	return hd;
}

bool harddisk_inspect_blockdevice(struct harddisk* hd)
{
	if ( !S_ISBLK(hd->st.st_mode) )
		return errno = ENOTBLK, false;
	if ( tcgetblob(hd->fd, "harddisk-driver", NULL, 0) < 0 )
	{
		if ( errno == ENOENT )
			errno = ENOTBLK;
		return false;
	}
	if ( !(hd->driver = atcgetblob(hd->fd, "harddisk-driver", NULL)) ||
	     !(hd->model = atcgetblob(hd->fd, "harddisk-model", NULL)) ||
	     !(hd->serial = atcgetblob(hd->fd, "harddisk-serial", NULL)) )
		return harddisk_close(hd), false;
	char* str;
	if ( !(str = atcgetblob(hd->fd, "harddisk-block-size", NULL)) )
		return harddisk_close(hd), false;
	hd->logical_block_size = (blksize_t) strtoumax(str, NULL, 10);
	free(str);
	if ( !(str = atcgetblob(hd->fd, "harddisk-cylinders", NULL)) )
		return harddisk_close(hd), false;
	hd->cylinders = (uint16_t) strtoul(str, NULL, 10);
	free(str);
	if ( !(str = atcgetblob(hd->fd, "harddisk-heads", NULL)) )
		return harddisk_close(hd), false;
	hd->heads = (uint16_t) strtoul(str, NULL, 10);
	free(str);
	if ( !(str = atcgetblob(hd->fd, "harddisk-sectors", NULL)) )
		return harddisk_close(hd), false;
	hd->sectors = (uint16_t) strtoul(str, NULL, 10);
	free(str);
	// TODO: To avoid potential overflow bugs (assuming malicious filesystem),
	//       reject ridiculous block sizes.
	if ( 65536 < hd->logical_block_size )
		return harddisk_close(hd), errno = EINVAL, false;
	if ( hd->logical_block_size < 512 )
		return harddisk_close(hd), errno = EINVAL, false;
	return true;
}

void harddisk_close(struct harddisk* hd)
{
	if ( 0 <= hd->fd )
		close(hd->fd);
	free(hd->path);
	free(hd->driver);
	free(hd->model);
	free(hd->serial);
	free(hd);
}
