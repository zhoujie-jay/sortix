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

    mount/ext2.h
    ext2 filesystem.

*******************************************************************************/

#ifndef INCLUDE_MOUNT_EXT2_H
#define INCLUDE_MOUNT_EXT2_H

#include <stdint.h>

#include <mount/filesystem.h>

static const uint16_t EXT2_SUPER_MAGIC = 0xEF53;
static const uint16_t EXT2_VALID_FS = 1;
static const uint16_t EXT2_ERROR_FS = 2;

struct ext2_superblock
{
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	 int32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint16_t s_def_resuid;
	uint16_t s_def_resgid;
// EXT2_DYNAMIC_REV
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
	uint8_t  s_uuid[16];
	/*uint8_t*/ char s_volume_name[16];
	/*uint8_t*/ char s_last_mounted[64];
	uint32_t s_algo_bitmap;
// Performance Hints
	uint8_t  s_prealloc_blocks;
	uint8_t  s_prealloc_dir_blocks;
	uint16_t alignment0;
// Journaling Support
	uint8_t  s_journal_uuid[16];
	uint32_t s_journal_inum;
	uint32_t s_journal_dev;
	uint32_t s_last_orphan;
// Directory Indexing Support
	uint32_t s_hash_seed[4];
	uint8_t  s_def_hash_version;
	uint8_t  alignment1[3];
// Other options
	uint32_t s_default_mount_options;
	uint32_t s_first_meta_bg;
	uint8_t  alignment2[760];
};

#if defined(__cplusplus)
extern "C" {
#endif

extern const struct filesystem_handler ext2_handler;

void ext2_superblock_decode(struct ext2_superblock*);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
