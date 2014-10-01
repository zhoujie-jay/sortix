/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    mbrfs.cpp
    Creates block devices representing master boot record partitions.

*******************************************************************************/

#include <endian.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <ioleast.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fsmarshall.h>

struct partition
{
	little_uint8_t flags;
	little_uint8_t start_head;
	little_uint16_t start_sector_cylinder;
	little_uint8_t system_id;
	little_uint8_t end_head;
	little_uint16_t end_sector_cylinder;
	little_uint32_t start_sector;
	little_uint32_t total_sectors;
} __attribute__((packed));

struct partition_lba48
{
	little_uint8_t flags;
	little_uint8_t signature1;
	little_uint16_t start_sector_high;
	little_uint8_t system_id;
	little_uint8_t signature2;
	little_uint16_t total_sectors_high;
	little_uint32_t start_sector;
	little_uint32_t total_sectors;
} __attribute__((packed));

struct mbr
{
	uint8_t bootstrap[436];
	uint8_t unique_disk_id[10];
	struct partition partitions[4];
	uint8_t signature[2];
} __attribute__((packed));

bool memiszero(const void* mem, size_t size)
{
	for ( size_t i = 0; i < size; i++ )
		if ( ((const uint8_t*) mem)[i] )
			return false;
	return true;
}

bool is_48bit_lba_partition(const struct partition* partition)
{
	const struct partition_lba48* partition_lba48 =
		(const struct partition_lba48*) partition;
	return partition_lba48->flags & 0x1 &&
	       partition_lba48->signature1 == 0x14 &&
	       partition_lba48->signature2 == 0xeb;
}

bool is_extended_partition(const struct partition* partition)
{
	return partition->system_id == 0x5 || partition->system_id == 0xF;
}

uint64_t partition_get_start(const struct partition* partition)
{
	if ( !is_48bit_lba_partition(partition) )
		return (uint64_t) partition->start_sector * 512;
	const struct partition_lba48* partition_lba48 =
		(const struct partition_lba48*) partition;
	uint64_t lower = partition_lba48->start_sector;
	uint64_t higher = partition_lba48->start_sector_high;
	return ((higher << 32) + lower) * 512;
}

uint64_t partition_get_length(const struct partition* partition)
{
	if ( !is_48bit_lba_partition(partition) )
		return (uint64_t) partition->total_sectors * 512;
	const struct partition_lba48* partition_lba48 =
		(const struct partition_lba48*) partition;
	uint64_t lower = partition_lba48->total_sectors;
	uint64_t higher = partition_lba48->total_sectors_high;
	return ((higher << 32) + lower) * 512;
}

bool is_partition_used(const struct partition* partition)
{
	if ( memiszero(partition, sizeof(*partition)) )
		return false;
	if ( !partition->system_id )
		return false;
	if ( !partition_get_start(partition) )
		return false;
	if ( !partition_get_length(partition) )
		return false;
	return true;
}

bool verify_is_partition(const struct partition* partition)
{
	if ( memiszero(partition, sizeof(*partition)) )
		return true;
	return true;
}

bool verify_is_mbr(const struct mbr* mbr)
{
	if ( memiszero(mbr, sizeof(*mbr)) )
		return false;
	if ( !(mbr->signature[0] == 0x55 && mbr->signature[1] == 0xAA) )
		return false;
	bool found_extended = false;
	for ( size_t i = 0; i < 4; i++ )
	{
		const struct partition* partition = &mbr->partitions[i];
		if ( !verify_is_partition(partition) )
			return false;
		if ( is_extended_partition(partition) )
		{
			if ( found_extended )
				return false;
			found_extended = true;
		}
	}
	return true;
}

int create_partition_block_device(const char* path, int diskfd, off_t start,
                                  off_t length)
{
	int ret = -1, mountfd, partfd;

	// Make sure that is a file that we can mount bind us on.
	if ( 0 <= (mountfd = open(path, O_RDONLY | O_CREAT, 0666)) )
	{
		// Create a file descriptor for our new partition.
		if ( 0 <= (partfd = mkpartition(diskfd, start, length)) )
		{
			ret = fsm_fsbind(partfd, mountfd, 0);
			close(partfd);
		}
		close(mountfd);
	}

	return ret;
}

void Usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [--probe] DEVICE...\n", argv0);
}

void Help(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
}

void Version(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool probe = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				Usage(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--usage") ) { Usage(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--probe") )
			probe = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			Usage(stderr, argv0);
			exit(1);
		}
	}

	if ( argc == 1 )
	{
		Usage(stderr, argv0);
		exit(0);
	}

	for ( int i = 1; i < argc; i++ )
	{
		if ( !argv[i] )
			continue;
		const char* path = argv[i];
		size_t path_len = strlen(path);
		int fd = open(path, O_RDWR);
		if ( fd < 0 )
			error(1, errno, "`%s'", path);
		struct mbr mbr;
		size_t amount = preadall(fd, &mbr, sizeof(mbr), 0);
		if ( amount < sizeof(mbr) && errno != EEOF )
			error(1, errno, "read: `%s'", path);
		if ( amount < sizeof(mbr) || !verify_is_mbr(&mbr) )
		{
			if ( probe )
				exit(1);
			error(1, 0, "`%s': No valid master boot record detected", path);
		}
		for ( size_t i = 0; !probe && i < 4; i++ )
		{
			struct partition* partition = &mbr.partitions[i];
			if ( !is_partition_used(partition) )
				continue;
			// TODO: Support extended partitions!
			if ( is_extended_partition(partition) )
				continue;
			uint64_t start = partition_get_start(partition);
			uint64_t length = partition_get_length(partition);

			size_t device_name_len = path_len + 1 + sizeof(size_t) * 3;
			char* device_name = new char[device_name_len+1];
			snprintf(device_name, device_name_len, "%sp%zu", path, i+1);

			if ( create_partition_block_device(device_name, fd, start, length) != 0 )
				error(1, errno, "creating `%s'", device_name);

			printf("%s\n", device_name);

			delete[] device_name;
		}
		close(fd);
	}
	return 0;
}
