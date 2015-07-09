/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

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

    extfs.cpp
    Implementation of the extended filesystem.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__sortix__)
#include "fsmarshall.h"
#else
#include "fuse.h"
#endif

#include "ext-constants.h"
#include "ext-structs.h"

#include "blockgroup.h"
#include "block.h"
#include "device.h"
#include "extfs.h"
#include "filesystem.h"
#include "inode.h"
#include "ioleast.h"

static const uint32_t EXT2_FEATURE_COMPAT_SUPPORTED = 0;
static const uint32_t EXT2_FEATURE_INCOMPAT_SUPPORTED = \
                      EXT2_FEATURE_INCOMPAT_FILETYPE;
static const uint32_t EXT2_FEATURE_RO_COMPAT_SUPPORTED = \
                      EXT2_FEATURE_RO_COMPAT_LARGE_FILE;

mode_t HostModeFromExtMode(uint32_t extmode)
{
	mode_t hostmode = extmode & 0777;
	if ( extmode & EXT2_S_ISVTX ) hostmode |= S_ISVTX;
	if ( extmode & EXT2_S_ISGID ) hostmode |= S_ISGID;
	if ( extmode & EXT2_S_ISUID ) hostmode |= S_ISUID;
	if ( EXT2_S_ISSOCK(extmode) ) hostmode |= S_IFSOCK;
	if ( EXT2_S_ISLNK(extmode) ) hostmode |= S_IFLNK;
	if ( EXT2_S_ISREG(extmode) ) hostmode |= S_IFREG;
	if ( EXT2_S_ISBLK(extmode) ) hostmode |= S_IFBLK;
	if ( EXT2_S_ISDIR(extmode) ) hostmode |= S_IFDIR;
	if ( EXT2_S_ISCHR(extmode) ) hostmode |= S_IFCHR;
	if ( EXT2_S_ISFIFO(extmode) ) hostmode |= S_IFIFO;
	return hostmode;
}

uint32_t ExtModeFromHostMode(mode_t hostmode)
{
	uint32_t extmode = hostmode & 0777;
	if ( hostmode & S_ISVTX ) extmode |= EXT2_S_ISVTX;
	if ( hostmode & S_ISGID ) extmode |= EXT2_S_ISGID;
	if ( hostmode & S_ISUID ) extmode |= EXT2_S_ISUID;
	if ( S_ISSOCK(hostmode) ) extmode |= EXT2_S_IFSOCK;
	if ( S_ISLNK(hostmode) ) extmode |= EXT2_S_IFLNK;
	if ( S_ISREG(hostmode) ) extmode |= EXT2_S_IFREG;
	if ( S_ISBLK(hostmode) ) extmode |= EXT2_S_IFBLK;
	if ( S_ISDIR(hostmode) ) extmode |= EXT2_S_IFDIR;
	if ( S_ISCHR(hostmode) ) extmode |= EXT2_S_IFCHR;
	if ( S_ISFIFO(hostmode) ) extmode |= EXT2_S_IFIFO;
	return extmode;
}

void StatInode(Inode* inode, struct stat* st)
{
	memset(st, 0, sizeof(*st));
	st->st_ino = inode->inode_id;
	st->st_mode = HostModeFromExtMode(inode->Mode());
	st->st_nlink = inode->data->i_links_count;
	st->st_uid = inode->UserId();
	st->st_gid = inode->GroupId();
	st->st_size = inode->Size();
	st->st_atim.tv_sec = inode->data->i_atime;
	st->st_atim.tv_nsec = 0;
	st->st_ctim.tv_sec = inode->data->i_ctime;
	st->st_ctim.tv_nsec = 0;
	st->st_mtim.tv_sec = inode->data->i_mtime;
	st->st_mtim.tv_nsec = 0;
	st->st_blksize = inode->filesystem->block_size;
	st->st_blocks = inode->data->i_blocks;
}

static bool is_hex_digit(char c)
{
	return ('0' <= c && c <= '9') ||
	       ('a' <= c && c <= 'f') ||
	       ('A' <= c && c <= 'F');
}

static bool is_valid_uuid(const char* uuid)
{
	if ( strlen(uuid) != 36 )
		return false;
	// Format: 01234567-0123-0123-0123-0123456789AB
	for ( size_t i = 0; i < 36; i++ )
	{
		if ( i == 8 || i == 13 || i == 18 || i == 23 )
		{
			if ( uuid[i] != '-' )
				return false;
		}
		else
		{
			if ( !is_hex_digit(uuid[i]) )
				return false;
		}
	}
	return true;
}

static unsigned char debase(char c)
{
	if ( '0' <= c && c <= '9' )
		return (unsigned char) (c - '0');
	if ( 'a' <= c && c <= 'f' )
		return (unsigned char) (c - 'a' + 10);
	if ( 'A' <= c && c <= 'F' )
		return (unsigned char) (c - 'A' + 10);
	return 0;
}

static void uuid_from_string(uint8_t uuid[16], const char* string)
{
	assert(is_valid_uuid(string));
	size_t output_index = 0;
	size_t i = 0;
	while ( i < 36 )
	{
		assert(string[i + 0] != '\0');
		if ( i == 8 || i == 13 || i == 18 || i == 23 )
		{
			i++;
			continue;
		}
		assert(string[i + 1] != '\0');
		uuid[output_index++] = debase(string[i + 0]) << 4 |
		                       debase(string[i + 1]) << 0;
		i += 2;
	}
	assert(string[i] == '\0');
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... DEVICE [MOUNT-POINT]\n", argv0);
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	const char* test_uuid = NULL;
	bool foreground = false;
	bool probe = false;
	bool read = false;
	bool write = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'r': read = true; break;
			case 'w': write = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--background") )
			foreground = false;
		else if ( !strcmp(arg, "--foreground") )
			foreground = true;
		else if ( !strcmp(arg, "--probe") )
			probe = true;
		else if ( !strcmp(arg, "--read") )
			read = true;
		else if ( !strcmp(arg, "--write") )
			write = true;
		else if ( !strcmp(arg, "--test-uuid") )
		{
			if ( i+1 == argc )
			{
				fprintf(stderr, "%s:  --test-uuid: Missing operand\n", argv0);
				exit(1);
			}
			test_uuid = argv[++i], argv[i] = NULL;
		}
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	// It doesn't make sense to have a write-only filesystem.
	read = read || write;

	// Default to read and write filesystem access.
	bool default_access = !read && !write ? read = write = true : false;

	if ( argc == 1 )
	{
		help(stdout, argv0);
		exit(0);
	}

	compact_arguments(&argc, &argv);

	const char* device_path = 2 <= argc ? argv[1] : NULL;
	const char* mount_path = 3 <= argc ? argv[2] : NULL;

	if ( !device_path )
	{
		help(stderr, argv0);
		exit(1);
	}

	int fd = open(device_path, write ? O_RDWR : O_RDONLY);
	if ( fd < 0 )
		error(1, errno, "`%s'", device_path);

	// Read the super block from the filesystem so we can verify it.
	struct ext_superblock sb;
	if ( preadall(fd, &sb, sizeof(sb), 1024) != sizeof(sb) )
	{
		if ( probe )
			exit(1);
		else if ( errno == EEOF )
			error(1, 0, "`%s' isn't a valid extended filesystem", device_path);
		else
			error(1, errno, "read: `%s'", device_path);
	}

	// Verify the magic value to detect a compatible filesystem.
	if ( !probe && sb.s_magic != EXT2_SUPER_MAGIC )
		error(1, 0, "`%s' isn't a valid extended filesystem", device_path);

	if ( probe && sb.s_magic != EXT2_SUPER_MAGIC )
		exit(1);

	// Test whether this was the filesystem the user was looking for.
	if ( test_uuid )
	{
		if ( !is_valid_uuid(test_uuid) )
		{
			if ( !probe )
				error(1, 0, "`%s' isn't a valid uuid", test_uuid);
			exit(1);
		}

		uint8_t uuid[16];
		uuid_from_string(uuid, test_uuid);

		if ( memcmp(sb.s_uuid, uuid, 16) != 0 )
		{
			if ( !probe )
				error(1, 0, "uuid `%s' did not match the ext2 filesystem at `%s'", test_uuid, device_path);
			exit(1);
		}
	}

	// Test whether this revision of the extended filesystem is supported.
	if ( sb.s_rev_level == EXT2_GOOD_OLD_REV )
	{
		if ( probe )
			exit(1);
		error(1, 0, "`%s' is formatted with an obsolete filesystem revision",
		            device_path);
	}

	// Verify that no incompatible features are in use.
	if ( sb.s_feature_compat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED )
	{
		if ( probe )
			exit(1);
		error(1, 0, "`%s' uses unsupported and incompatible features",
		            device_path);
	}

	// Verify that no incompatible features are in use if opening for write.
	if ( !default_access && write &&
	     sb.s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED )
	{
		if ( probe )
			exit(1);
		error(1, 0, "`%s' uses unsupported and incompatible features, "
		            "read-only access is possible, but write-access was "
		            "requested", device_path);
	}

	if ( write && sb.s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED )
	{
		if ( !probe )
			fprintf(stderr, "Warning: `%s' uses unsupported and incompatible "
			                "features, falling back to read-only access\n",
			                device_path);
		// TODO: Modify the file descriptor such that writing fails!
		read = false;
	}

	// Check whether any features are in use that we can safely disregard.
	if ( !probe && sb.s_feature_compat & ~EXT2_FEATURE_COMPAT_SUPPORTED )
		fprintf(stderr, "Note: filesystem uses unsupported but compatible "
		                "features\n");

	// Check the block size is sane. 64 KiB may have issues, 32 KiB then.
	if ( sb.s_log_block_size > (15-10) /* 32 KiB blocks */ )
	{
		if ( probe )
			exit(1);
		error(1, 0, "`%s': excess block size", device_path);
	}

	// We have found no critical problems, so let the caller know that this
	// filesystem satisfies the probe request.
	if ( probe )
		exit(0);

	// Check whether the filesystem was unmounted cleanly.
	if ( sb.s_state != EXT2_VALID_FS )
		fprintf(stderr, "Warning: `%s' wasn't unmounted cleanly\n",
	                    device_path);

	uint32_t block_size = 1024U << sb.s_log_block_size;

	Device* dev = new Device(fd, device_path, block_size, write);
	Filesystem* fs = new Filesystem(dev, mount_path);

	fs->block_groups = new BlockGroup*[fs->num_groups];
	for ( size_t i = 0; i < fs->num_groups; i++ )
		fs->block_groups[i] = NULL;

	if ( !mount_path )
		return 0;

#if defined(__sortix__)
	return fsmarshall_main(argv0, mount_path, foreground, fs, dev);
#else
	return ext2_fuse_main(argv0, mount_path, foreground, fs, dev);
#endif
}
