/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ext2.c
 * ext2 filesystem.
 */

#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <mount/blockdevice.h>
#include <mount/ext2.h>
#include <mount/filesystem.h>

#include "util.h"

void ext2_superblock_decode(struct ext2_superblock* sb)
{
	sb->s_inodes_count = le32toh(sb->s_inodes_count);
	sb->s_blocks_count = le32toh(sb->s_blocks_count);
	sb->s_r_blocks_count = le32toh(sb->s_r_blocks_count);
	sb->s_free_blocks_count = le32toh(sb->s_free_blocks_count);
	sb->s_free_inodes_count = le32toh(sb->s_free_inodes_count);
	sb->s_first_data_block = le32toh(sb->s_first_data_block);
	sb->s_log_block_size = le32toh(sb->s_log_block_size);
	sb->s_log_frag_size = (int32_t) le32toh((uint32_t) sb->s_log_frag_size);
	sb->s_blocks_per_group = le32toh(sb->s_blocks_per_group);
	sb->s_frags_per_group = le32toh(sb->s_frags_per_group);
	sb->s_inodes_per_group = le32toh(sb->s_inodes_per_group);
	sb->s_mtime = le32toh(sb->s_mtime);
	sb->s_wtime = le32toh(sb->s_wtime);
	sb->s_mnt_count = le16toh(sb->s_mnt_count);
	sb->s_max_mnt_count = le16toh(sb->s_max_mnt_count);
	sb->s_magic = le16toh(sb->s_magic);
	sb->s_state = le16toh(sb->s_state);
	sb->s_errors = le16toh(sb->s_errors);
	sb->s_minor_rev_level = le16toh(sb->s_minor_rev_level);
	sb->s_lastcheck = le32toh(sb->s_lastcheck);
	sb->s_checkinterval = le32toh(sb->s_checkinterval);
	sb->s_creator_os = le32toh(sb->s_creator_os);
	sb->s_rev_level = le32toh(sb->s_rev_level);
	sb->s_def_resuid = le16toh(sb->s_def_resuid);
	sb->s_def_resgid = le16toh(sb->s_def_resgid);
	sb->s_first_ino = le32toh(sb->s_first_ino);
	sb->s_inode_size = le16toh(sb->s_inode_size);
	sb->s_block_group_nr = le16toh(sb->s_block_group_nr);
	sb->s_feature_compat = le32toh(sb->s_feature_compat);
	sb->s_feature_incompat = le32toh(sb->s_feature_incompat);
	sb->s_feature_ro_compat = le32toh(sb->s_feature_ro_compat);
	// s_uuid is endian agnostic.
	// s_volume_name is endian agnostic.
	// s_last_mounted is endian agnostic.
	sb->s_algo_bitmap = le32toh(sb->s_algo_bitmap);
	// s_prealloc_blocks is endian agnostic.
	// s_prealloc_dir_blocks is endian agnostic.
	sb->alignment0 = le16toh(sb->alignment0);
	// s_journal_uuid is endian agnostic.
	sb->s_journal_inum = le32toh(sb->s_journal_inum);
	sb->s_journal_dev = le32toh(sb->s_journal_dev);
	sb->s_last_orphan = le32toh(sb->s_last_orphan);
	for ( size_t i = 0; i < 4; i++ )
		sb->s_hash_seed[i] = le32toh(sb->s_hash_seed[i]);
	// s_def_hash_version is endian agnostic.
	// alignment1 is endian agnostic.
	sb->s_default_mount_options = le32toh(sb->s_default_mount_options);
	sb->s_first_meta_bg = le32toh(sb->s_first_meta_bg);
}

static size_t ext2_probe_amount(struct blockdevice* bdev)
{
	(void) bdev;
	return 1024 + sizeof(struct ext2_superblock);
}

static bool ext2_probe(struct blockdevice* bdev,
                       const unsigned char* leading,
                       size_t amount)
{
	(void) bdev;
	if ( amount < 1024 )
		return false;
	leading += 1024;
	amount -= 1024;
	if ( amount < sizeof(struct ext2_superblock) )
		return false;
	struct ext2_superblock sb;
	memcpy(&sb, leading, sizeof(sb));
	ext2_superblock_decode(&sb);
	if ( sb.s_magic != EXT2_SUPER_MAGIC )
		return false;
	return true;
}

struct ext2_private
{
	struct ext2_superblock sb;
};

static void ext2_release(struct filesystem* fs)
{
	if ( !fs )
		return;
	struct ext2_private* priv = (struct ext2_private*) fs->handler_private;
	free(priv);
	free(fs);
}

static enum filesystem_error ext2_inspect(struct filesystem** fs_ptr,
                                          struct blockdevice* bdev)
{
	*fs_ptr = NULL;
	struct filesystem* fs = CALLOC_TYPE(struct filesystem);
	if ( !fs )
		return FILESYSTEM_ERROR_ERRNO;
	struct ext2_private* priv = CALLOC_TYPE(struct ext2_private);
	if ( !priv )
		return free(fs), FILESYSTEM_ERROR_ERRNO;
	fs->bdev = bdev;
	fs->handler = &ext2_handler;
	fs->handler_private = priv;
	struct ext2_superblock* sb = &priv->sb;
	if ( blockdevice_preadall(bdev, sb, sizeof(*sb), 1024) != sizeof(*sb) )
		return ext2_release(fs), FILESYSTEM_ERROR_ERRNO;
	ext2_superblock_decode(sb);
	fs->fstype_name = "ext2";
	fs->fsck = "fsck.ext2";
	fs->driver = "extfs";
	fs->flags |= FILESYSTEM_FLAG_UUID;
	memcpy(&fs->uuid, sb->s_uuid, 16);
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	time_t next_check = (time_t)
		((uintmax_t) sb->s_lastcheck + (uintmax_t) sb->s_checkinterval);
	if ( sb->s_state != EXT2_VALID_FS )
		fs->flags |= FILESYSTEM_FLAG_FSCK_SHOULD |
		             FILESYSTEM_FLAG_FSCK_MUST;
	else if ( sb->s_max_mnt_count && sb->s_max_mnt_count <= sb->s_mnt_count )
		fs->flags |= FILESYSTEM_FLAG_FSCK_SHOULD;
	else if ( sb->s_checkinterval && next_check <= now.tv_sec )
		fs->flags |= FILESYSTEM_FLAG_FSCK_SHOULD;
	return *fs_ptr = fs, FILESYSTEM_ERROR_NONE;
}

const struct filesystem_handler ext2_handler =
{
	.handler_name = "ext2",
	.probe_amount = ext2_probe_amount,
	.probe = ext2_probe,
	.inspect = ext2_inspect,
	.release = ext2_release,
};
