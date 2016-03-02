/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * inode.cpp
 * Filesystem inode.
 */

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "block.h"
#include "blockgroup.h"
#include "device.h"
#include "extfs.h"
#include "filesystem.h"
#include "inode.h"
#include "util.h"

#ifndef S_SETABLE
#define S_SETABLE 02777
#endif
#ifndef O_WRITE
#define O_WRITE (O_WRONLY | O_RDWR)
#endif

Inode::Inode(Filesystem* filesystem, uint32_t inode_id)
{
	this->prev_inode = NULL;
	this->next_inode = NULL;
	this->prev_hashed = NULL;
	this->next_hashed = NULL;
	this->prev_dirty = NULL;
	this->next_dirty = NULL;
	this->data_block = NULL;
	this->data = NULL;
	this->filesystem = filesystem;
	this->reference_count = 1;
	this->remote_reference_count = 0;
	this->inode_id = inode_id;
	this->dirty = false;
}

Inode::~Inode()
{
	Sync();
	if ( data_block )
		data_block->Unref();
	Unlink();
}

uint32_t Inode::Mode()
{
	// TODO: Use i_mode_high.
	return data->i_mode;
}

void Inode::SetMode(uint32_t mode)
{
	assert(filesystem->device->write);
	BeginWrite();
	// TODO: Use i_mode_high.
	data->i_mode = (uint16_t) mode;
	FinishWrite();
}

uint32_t Inode::UserId()
{
	// TODO: Use i_uid_high.
	return data->i_uid;
}

void Inode::SetUserId(uint32_t user)
{
	assert(filesystem->device->write);
	BeginWrite();
	// TODO: Use i_uid_high.
	data->i_uid = (uint16_t) user;
	FinishWrite();
}

uint32_t Inode::GroupId()
{
	// TODO: Use i_gid_high.
	return data->i_gid;
}

void Inode::SetGroupId(uint32_t group)
{
	assert(filesystem->device->write);
	BeginWrite();
	// TODO: Use i_gid_high.
	data->i_gid = (uint16_t) group;
	FinishWrite();
}

uint64_t Inode::Size()
{
	bool largefile = filesystem->sb->s_feature_ro_compat &
	                 EXT2_FEATURE_RO_COMPAT_LARGE_FILE;
	if ( !EXT2_S_ISREG(data->i_mode) || !largefile )
		return data->i_size;
	uint64_t lower = data->i_size;
	uint64_t upper = data->i_dir_acl;
	uint64_t size = lower | (upper << 32ULL);
	return size;
}

void Inode::SetSize(uint64_t new_size)
{
	assert(filesystem->device->write);
	bool largefile = filesystem->sb->s_feature_ro_compat &
	                 EXT2_FEATURE_RO_COMPAT_LARGE_FILE;
	uint32_t lower = new_size & 0xFFFFFFFF;
	uint32_t upper = new_size >> 32;
	// TODO: Enforce filesize limit with no largefile support or non-files.
	data->i_size = lower;

	// TODO: Figure out these i_blocks semantics and how stuff is reserved.
	const uint64_t ENTRIES = filesystem->block_size / sizeof(uint32_t);
	uint64_t block_direct = sizeof(data->i_block) / sizeof(uint32_t) - 3;
	uint64_t block_singly = ENTRIES;
	uint64_t block_doubly = block_singly * block_singly;
	uint64_t max_direct = block_direct;
	uint64_t max_singly = max_direct + block_singly;
	uint64_t max_doubly = max_singly + block_doubly;
	uint64_t logical_blocks = divup(new_size, (uint64_t) filesystem->block_size);
	uint64_t actual_blocks  = logical_blocks;
	if ( max_direct <= logical_blocks )
		actual_blocks += divup(logical_blocks - max_direct, ENTRIES);
	if ( max_singly <= logical_blocks )
		actual_blocks += divup(logical_blocks - max_singly, ENTRIES * ENTRIES);
	if ( max_doubly <= logical_blocks )
		actual_blocks += divup(logical_blocks - max_doubly, ENTRIES * ENTRIES * ENTRIES);

	BeginWrite();
	data->i_blocks = (actual_blocks * filesystem->block_size) / 512;
	if ( EXT2_S_ISREG(data->i_mode) && largefile )
		data->i_dir_acl = upper;
	FinishWrite();

	Modified();
}

Block* Inode::GetBlockFromTable(Block* table, uint32_t index)
{
	if ( uint32_t block_id = ((uint32_t*) table->block_data)[index] )
		return filesystem->device->GetBlock(block_id);
	// TODO: If in read only mode, then perhaps return a zero block here.
	if ( !filesystem->device->write )
		return NULL;
	uint32_t group_id = (inode_id - 1) / filesystem->sb->s_inodes_per_group;
	assert(group_id < filesystem->num_groups);
	BlockGroup* block_group = filesystem->GetBlockGroup(group_id);
	uint32_t block_id = filesystem->AllocateBlock(block_group);
	block_group->Unref();
	if ( block_id )
	{
		Block* block = filesystem->device->GetBlockZeroed(block_id);
		table->BeginWrite();
		((uint32_t*) table->block_data)[index] = block_id;
		table->FinishWrite();
		return block;
	}
	return NULL;
}

Block* Inode::GetBlock(uint64_t offset)
{
	const uint64_t ENTRIES = filesystem->block_size / sizeof(uint32_t);
	uint64_t block_direct = sizeof(data->i_block) / sizeof(uint32_t) - 3;
	uint64_t block_singly = ENTRIES;
	uint64_t block_doubly = block_singly * block_singly;
	uint64_t block_triply = block_doubly * block_singly;
	uint64_t max_direct = block_direct;
	uint64_t max_singly = max_direct + block_singly;
	uint64_t max_doubly = max_singly + block_doubly;
	uint64_t max_triply = max_doubly + block_triply;
	uint32_t index;

	Block* table = data_block; table->Refer();
	Block* block;

	uint32_t inode_offset = (uintptr_t) data - (uintptr_t) data_block->block_data;
	uint32_t table_offset = offsetof(struct ext_inode, i_block);
	uint32_t inode_block_table_offset = (inode_offset + table_offset) / sizeof(uint32_t);

	// TODO: It would seem that somebody needs a lesson in induction. :-)
	if ( offset < max_direct )
	{
		offset -= 0;
		offset += inode_block_table_offset * 1;
	read_direct:
		index = offset;
		offset %= 1;
		block = GetBlockFromTable(table, index);
		table->Unref();
		if ( !block )
			return NULL;
		return block;
	}
	else if ( offset < max_singly )
	{
		offset -= max_direct;
		offset += (inode_block_table_offset + 12) * ENTRIES;
	read_singly:
		index = offset / ENTRIES;
		offset = offset % ENTRIES;
		block = GetBlockFromTable(table, index);
		table->Unref();
		if ( !block )
			return NULL;
		table = block;
		goto read_direct;
	}
	else if ( offset < max_doubly )
	{
		offset -= max_singly;
		offset += (inode_block_table_offset + 13) * ENTRIES * ENTRIES;
	read_doubly:
		index = offset / (ENTRIES * ENTRIES);
		offset = offset % (ENTRIES * ENTRIES);
		block = GetBlockFromTable(table, index);
		table->Unref();
		if ( !block )
			return NULL;
		table = block;
		goto read_singly;
	}
	else if ( offset < max_triply )
	{
		offset -= max_doubly;
		offset += (inode_block_table_offset + 14) * ENTRIES * ENTRIES * ENTRIES;
	/*read_triply:*/
		index = offset / (ENTRIES * ENTRIES * ENTRIES);
		offset = offset % (ENTRIES * ENTRIES * ENTRIES);
		block = GetBlockFromTable(table, index);
		table->Unref();
		if ( !block )
			return NULL;
		table = block;
		goto read_doubly;
	}

	table->Unref();
	return NULL;
}

bool Inode::FreeIndirect(uint64_t from, uint64_t offset, uint32_t block_id,
                         int indirection, uint64_t entry_span)
{
	assert(filesystem->device->write);
	const uint64_t ENTRIES = filesystem->block_size / sizeof(uint32_t);
	Block* block = filesystem->device->GetBlock(block_id);
	uint32_t* table = (uint32_t*) block->block_data;
	bool any_children = false;
	bool begun_writing = false;
	for ( uint64_t i = 0; i < ENTRIES; i++ )
	{
		if ( !table[i] )
			continue;
		uint64_t entry_offset = offset + entry_span * i;
		if ( entry_offset < from ||
		     (indirection &&
		      FreeIndirect(from, entry_offset, table[i], indirection-1,
		                   entry_span / ENTRIES)) )
		{
			any_children = true;
			continue;
		}
		filesystem->FreeBlock(table[i]);
		if ( !begun_writing )
		{
			block->BeginWrite();
			begun_writing = true;
		}
		table[i] = 0;
	}
	if ( begun_writing )
		block->FinishWrite();
	return any_children;
}

void Inode::Truncate(uint64_t new_size)
{
	assert(filesystem->device->write);
	uint64_t old_size = Size();
	bool could_be_embedded = old_size == 0 && EXT2_S_ISLNK(Mode()) && !data->i_blocks;
	bool is_embedded = 0 < old_size && old_size <= 60 && !data->i_blocks;
	if ( could_be_embedded || is_embedded )
	{
		if ( new_size <= 60 )
		{
			data_block->BeginWrite();
			unsigned char* block_data = (unsigned char*) &data->i_block[0];
			if ( old_size < new_size )
				memset(block_data + old_size, 0, new_size - old_size);
			if ( new_size < old_size )
				memset(block_data + new_size, 0, old_size - new_size);
			data->i_size = new_size;
			data_block->FinishWrite();
			return;
		}
		if ( is_embedded && !UnembedInInode() )
		{
			// Truncate() can't fail ATM so lose data instead of worse stuff.
			data_block->BeginWrite();
			unsigned char* block_data = (unsigned char*) &data->i_block[0];
			memset(block_data, 0, 60);
			data->i_size = 0;
			data_block->FinishWrite();
		}
	}
	// TODO: Enforce a filesize limit!
	SetSize(new_size);
	if ( old_size <= new_size )
		return;

	uint64_t old_num_blocks = divup(old_size, (uint64_t) filesystem->block_size);
	uint64_t new_num_blocks = divup(new_size, (uint64_t) filesystem->block_size);

	// Zero out the rest of the last partial block, if any.
	uint32_t partial = new_size % filesystem->block_size;
	if ( partial )
	{
		Block* partial_block = GetBlock(new_num_blocks-1);
		uint8_t* data = partial_block->block_data;
		partial_block->BeginWrite();
		memset(data + partial, 0, filesystem->block_size - partial);
		partial_block->FinishWrite();
	}

	const uint64_t ENTRIES = filesystem->block_size / sizeof(uint32_t);
	uint64_t block_direct = sizeof(data->i_block) / sizeof(uint32_t) - 3;
	uint64_t block_singly = ENTRIES;
	uint64_t block_doubly = block_singly * block_singly;
	uint64_t block_triply = block_doubly * block_singly;
	uint64_t max_direct = block_direct;
	uint64_t max_singly = max_direct + block_singly;
	uint64_t max_doubly = max_singly + block_doubly;
	uint64_t max_triply = max_doubly + block_triply;

	BeginWrite();

	for ( uint64_t i = new_num_blocks; i < old_num_blocks && i < 12; i++ )
	{
		if ( !data->i_block[i] )
			continue;
		filesystem->FreeBlock(data->i_block[i]);
		data->i_block[i] = 0;
	}

	if ( data->i_block[12] && !FreeIndirect(new_num_blocks, max_direct, data->i_block[12], 0, 1) )
	{
		filesystem->FreeBlock(data->i_block[12]);
		data->i_block[12] = 0;
	}

	if ( data->i_block[13] && !FreeIndirect(new_num_blocks, max_singly, data->i_block[13], 1, ENTRIES) )
	{
		filesystem->FreeBlock(data->i_block[13]);
		data->i_block[13] = 0;
	}

	if ( data->i_block[14] && !FreeIndirect(new_num_blocks, max_doubly, data->i_block[14], 2, ENTRIES * ENTRIES) )
	{
		filesystem->FreeBlock(data->i_block[14]);
		data->i_block[14] = 0;
	}

	(void) max_triply;

	FinishWrite();
}

Inode* Inode::Open(const char* elem, int flags, mode_t mode)
{
	if ( !EXT2_S_ISDIR(Mode()) )
		return errno = ENOTDIR, (Inode*) NULL;
	size_t elem_length = strlen(elem);
	if ( elem_length == 0 )
		return errno = ENOENT, (Inode*) NULL;
	uint64_t filesize = Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	while ( offset < filesize )
	{
		uint64_t entry_block_id = offset / filesystem->block_size;
		uint64_t entry_block_offset = offset % filesystem->block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = GetBlock(block_id = entry_block_id)) )
			return NULL;
		const uint8_t* block_data = block->block_data + entry_block_offset;
		const struct ext_dirent* entry = (const struct ext_dirent*) block_data;
		if ( entry->inode &&
		     entry->name_len == elem_length &&
		     memcmp(elem, entry->name, elem_length) == 0 )
		{
			uint8_t file_type = EXT2_FT_UNKNOWN;
			if ( filesystem->sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE )
				file_type = entry->file_type;
			uint32_t inode_id = entry->inode;
			block->Unref();
			if ( (flags & O_CREAT) && (flags & O_EXCL) )
				return errno = EEXIST, (Inode*) NULL;
			if ( (flags & O_DIRECTORY) &&
			     file_type != EXT2_FT_UNKNOWN &&
			     file_type != EXT2_FT_DIR &&
			     file_type != EXT2_FT_SYMLINK )
				return errno = ENOTDIR, (Inode*) NULL;
			Inode* inode = filesystem->GetInode(inode_id);
			if ( !inode )
				return (Inode*) NULL;
			if ( flags & O_DIRECTORY &&
			     !EXT2_S_ISDIR(inode->Mode()) &&
			     !EXT2_S_ISLNK(inode->Mode()) )
			{
				inode->Unref();
				return errno = ENOTDIR, (Inode*) NULL;
			}
			if ( flags & O_WRITE && !filesystem->device->write )
			{
				inode->Unref();
				return errno = EROFS, (Inode*) NULL;
			}
			if ( S_ISREG(inode->Mode()) && flags & O_WRITE && flags & O_TRUNC )
				inode->Truncate(0);
			return inode;
		}
		offset += entry->reclen;
	}
	if ( block )
		block->Unref();
	if ( flags & O_CREAT )
	{
		if ( !filesystem->device->write )
			return errno = EROFS, (Inode*) NULL;
		// TODO: Preferred block group!
		uint32_t result_inode_id = filesystem->AllocateInode();
		if ( !result_inode_id )
			return NULL;
		Inode* result = filesystem->GetInode(result_inode_id);
		if ( !result )
			return filesystem->FreeInode(result_inode_id), (Inode*) NULL;
		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		result->BeginWrite();
		memset(result->data, 0, sizeof(*result->data));
		result->data->i_atime = now.tv_sec;
		result->data->i_ctime = now.tv_sec;
		result->data->i_mtime = now.tv_sec;
		// TODO: Set all the other inode properties!
		result->FinishWrite();
		result->SetMode((mode & S_SETABLE) | S_IFREG);
		result->SetUserId(request_uid);
		result->SetGroupId(request_gid);
		if ( !Link(elem, result, false) )
		{
			result->Unref();
			return NULL;
		}
		return result;
	}
	return errno = ENOENT, (Inode*) NULL;
}

bool Inode::Link(const char* elem, Inode* dest, bool directories)
{
	if ( !EXT2_S_ISDIR(Mode()) )
		return errno = ENOTDIR, false;
	if ( directories && !EXT2_S_ISDIR(dest->Mode()) )
		return errno = ENOTDIR, false;
	if ( !directories && EXT2_S_ISDIR(dest->Mode()) )
		return errno = EISDIR, false;

	// Search for a hole in which we can store the new directory entry and stop
	// if we meet an existing link with the requested name.
	size_t elem_length = strlen(elem);
	if ( elem_length == 0 )
		return errno = ENOENT, false;
	size_t new_entry_size = roundup(sizeof(struct ext_dirent) + elem_length, (size_t) 4);
	uint64_t filesize = Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	bool found_hole = false;
	bool splitting = false;
	uint64_t hole_block_id = 0;
	uint64_t hole_block_offset = 0;
	while ( offset < filesize )
	{
		uint64_t entry_block_id = offset / filesystem->block_size;
		uint64_t entry_block_offset = offset % filesystem->block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = GetBlock(block_id = entry_block_id)) )
			return NULL;
		const uint8_t* block_data = block->block_data + entry_block_offset;
		const struct ext_dirent* entry = (const struct ext_dirent*) block_data;
		if ( entry->name_len == elem_length &&
		     memcmp(elem, entry->name, elem_length) == 0 )
		{
			block->Unref();
			return errno = EEXIST, false;
		}
		if ( !found_hole )
		{
			size_t entry_size = roundup(sizeof(struct ext_dirent) + entry->name_len, (size_t) 4);
			if ( (!entry->inode || !entry->name[0]) && new_entry_size <= entry->reclen )
			{
				hole_block_id = entry_block_id;
				hole_block_offset = entry_block_offset;
				new_entry_size = entry->reclen;
				found_hole = true;
			}
			else if ( new_entry_size <= entry->reclen - entry_size )
			{
				hole_block_id = entry_block_id;
				hole_block_offset = entry_block_offset;
				new_entry_size = entry->reclen - entry_size;
				splitting = true;
				found_hole = true;
			}
		}
		offset += entry->reclen;
	}

	if ( !filesystem->device->write )
		return errno = EROFS, false;

	if ( UINT16_MAX <= dest->data->i_links_count )
		return errno = EMLINK, false;

	if ( 255 < elem_length )
		return errno = ENAMETOOLONG, false;

	// We'll append another block if we failed to find a suitable hole.
	if ( !found_hole )
	{
		hole_block_id = filesize / filesystem->block_size;
		hole_block_offset = filesize % filesystem->block_size;
		new_entry_size = filesystem->block_size;
	}

	if ( block && block_id != hole_block_id )
		block->Unref(),
		block = NULL;
	if ( !block && !(block = GetBlock(block_id = hole_block_id)) )
		return NULL;

	Modified();

	block->BeginWrite();

	uint8_t* block_data = block->block_data + hole_block_offset;
	struct ext_dirent* entry = (struct ext_dirent*) block_data;

	// If we found a directory entry with room at the end, split it!
	if ( splitting )
	{
		entry->reclen = roundup(sizeof(struct ext_dirent) + entry->name_len, (size_t) 4);
		assert(entry->reclen);
		block_data += entry->reclen;
		entry = (struct ext_dirent*) block_data;
	}

	// Write the new directory entry.
	entry->inode = dest->inode_id;
	entry->reclen = new_entry_size;
	entry->name_len = elem_length;
	if ( filesystem->sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE )
		entry->file_type = EXT2_FT_OF_MODE(dest->Mode());
	else
		entry->file_type = EXT2_FT_UNKNOWN;
	strncpy(entry->name, elem, new_entry_size - sizeof(struct ext_dirent));

	block->FinishWrite();
	block->Unref();

	dest->BeginWrite();
	dest->data->i_links_count++;
	dest->FinishWrite();

	if ( !found_hole )
		SetSize(Size() + filesystem->block_size);

	return true;
}

Inode* Inode::UnlinkKeep(const char* elem, bool directories, bool force)
{
	if ( !EXT2_S_ISDIR(Mode()) )
		return errno = ENOTDIR, (Inode*) NULL;
	size_t elem_length = strlen(elem);
	if ( elem_length == 0 )
		return errno = ENOENT, (Inode*) NULL;
	uint32_t block_size = filesystem->block_size;
	uint64_t filesize = Size();
	uint64_t num_blocks = divup(filesize, (uint64_t) block_size);
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	struct ext_dirent* last_entry = NULL;
	while ( offset < filesize )
	{
		uint64_t entry_block_id = offset / block_size;
		uint64_t entry_block_offset = offset % block_size;
		if ( block && block_id != entry_block_id )
			last_entry = NULL,
			block->Unref(),
			block = NULL;
		if ( !block && !(block = GetBlock(block_id = entry_block_id)) )
			return NULL;
		uint8_t* block_data = block->block_data + entry_block_offset;
		struct ext_dirent* entry = (struct ext_dirent*) block_data;
		if ( entry->inode &&
		     entry->name_len == elem_length &&
		     memcmp(elem, entry->name, elem_length) == 0 )
		{
			Inode* inode = filesystem->GetInode(entry->inode);

			if ( !force && directories && !EXT2_S_ISDIR(inode->Mode()) )
			{
				inode->Unref();
				block->Unref();
				return errno = ENOTDIR, (Inode*) NULL;
			}

			if ( !force && directories && !inode->IsEmptyDirectory() )
			{
				inode->Unref();
				block->Unref();
				return errno = ENOTEMPTY, (Inode*) NULL;
			}

			if ( !force && !directories && EXT2_S_ISDIR(inode->Mode()) )
			{
				inode->Unref();
				block->Unref();
				return errno = EISDIR, (Inode*) NULL;
			}

			if ( !filesystem->device->write )
			{
				inode->Unref();
				block->Unref();
				return errno = EROFS, (Inode*) NULL;
			}

			Modified();

			inode->BeginWrite();
			inode->data->i_links_count--;
			inode->FinishWrite();

			block->BeginWrite();

			entry->inode = 0;
			entry->name_len = 0;
			entry->file_type = 0;

			// Merge the current entry with the previous if any.
			if ( last_entry )
			{
				last_entry->reclen += entry->reclen;
				memset(entry, 0, entry->reclen);
				entry = last_entry;
			}

			strncpy(entry->name + entry->name_len, "",
			        entry->reclen - sizeof(struct ext_dirent) - entry->name_len);

			// If the entire block is empty, we'll need to remove it.
			if ( !entry->name[0] && entry->reclen == block_size )
			{
				// If this is not the last block, we'll make it. This is faster
				// than shifting the entire directory a single block. We don't
				// actually copy this block to the end, since we'll truncate it
				// regardless.
				if ( entry_block_id + 1 != num_blocks )
				{
					Block* last_block = GetBlock(num_blocks-1);
					if ( last_block )
					{
						memcpy(block->block_data, last_block->block_data, block_size);
						last_block->Unref();
						Truncate(filesize - block_size);
					}
				}
				else
				{
					Truncate(filesize - block_size);
				}
			}

			block->FinishWrite();

			block->Unref();

			return inode;
		}
		offset += entry->reclen;
		last_entry = entry;
	}
	if ( block )
		block->Unref();
	return errno = ENOENT, (Inode*) NULL;
}

bool Inode::Unlink(const char* elem, bool directories, bool force)
{
	Inode* result = UnlinkKeep(elem, directories, force);
	if ( !result )
		return false;
	result->Unref();
	return true;
}

ssize_t Inode::ReadAt(uint8_t* buf, size_t s_count, off_t o_offset)
{
	if ( !EXT2_S_ISREG(Mode()) && !EXT2_S_ISLNK(Mode()) )
		return errno = EISDIR, -1;
	if ( o_offset < 0 )
		return errno = EINVAL, -1;
	if ( SSIZE_MAX < s_count )
		s_count = SSIZE_MAX;
	uint64_t sofar = 0;
	uint64_t count = (uint64_t) s_count;
	uint64_t offset = (uint64_t) o_offset;
	uint64_t file_size = Size();
	if ( file_size <= offset )
		return 0;
	if ( file_size - offset < count )
		count = file_size - offset;
	// TODO: This case also needs to be handled in SetSize, Truncate, WriteAt,
	//       and so on.
	if ( 0 < file_size && file_size <= 60 && !data->i_blocks )
	{
		assert(offset + count <= 60);
		unsigned char* block_data = (unsigned char*) &data->i_block[0];
		memcpy(buf, block_data + offset, count);
		return (ssize_t) count;
	}
	while ( sofar < count )
	{
		uint64_t block_id = offset / filesystem->block_size;
		uint32_t block_offset = offset % filesystem->block_size;
		uint32_t block_left = filesystem->block_size - block_offset;
		Block* block = GetBlock(block_id);
		if ( !block )
			return sofar ? sofar : -1;
		size_t amount = count - sofar < block_left ? count - sofar : block_left;
		memcpy(buf + sofar, block->block_data + block_offset, amount);
		sofar += amount;
		offset += amount;
		block->Unref();
	}
	return (ssize_t) sofar;
}

ssize_t Inode::WriteAt(const uint8_t* buf, size_t s_count, off_t o_offset)
{
	if ( !EXT2_S_ISREG(Mode()) && !EXT2_S_ISLNK(Mode()) )
		return errno = EISDIR, -1;
	if ( o_offset < 0 )
		return errno = EINVAL, -1;
	if ( !filesystem->device->write )
		return errno = EROFS, -1;
	if ( SSIZE_MAX < s_count )
		s_count = SSIZE_MAX;
	Modified();
	uint64_t sofar = 0;
	uint64_t count = (uint64_t) s_count;
	uint64_t offset = (uint64_t) o_offset;
	uint64_t file_size = Size();
	uint64_t end_at = offset + count;
	if ( offset < end_at )
		/* TODO: Overflow! off_t overflow? */{};
	if ( file_size < end_at )
		Truncate(end_at);
	if ( 0 < end_at && end_at <= 60 && !data->i_blocks )
	{
		data_block->BeginWrite();
		unsigned char* block_data = (unsigned char*) &data->i_block[0];
		memcpy(block_data + offset, buf, count);
		if ( data->i_size < end_at )
			data->i_size = end_at;
		data_block->FinishWrite();
		return (ssize_t) count;
	}
	while ( sofar < count )
	{
		uint64_t block_id = offset / filesystem->block_size;
		uint32_t block_offset = offset % filesystem->block_size;
		uint32_t block_left = filesystem->block_size - block_offset;
		Block* block = GetBlock(block_id);
		if ( !block )
			return sofar ? (ssize_t) sofar : -1;
		size_t amount = count - sofar < block_left ? count - sofar : block_left;
		block->BeginWrite();
		memcpy(block->block_data + block_offset, buf + sofar, amount);
		block->FinishWrite();
		block->Unref();
		sofar += amount;
		offset += amount;
	}
	return (ssize_t) sofar;
}

bool Inode::UnembedInInode()
{
	assert(data->i_blocks == 0 && 0 < data->i_size && data->i_size <= 60);
	unsigned char* block_data = (unsigned char*) &data->i_block[0];
	size_t content_size = Size();
	unsigned char content[60];
	memcpy(content, block_data, content_size);
	data_block->BeginWrite();
	memset(block_data, 0, 60);
	data->i_size = 0;
	data_block->FinishWrite();
	if ( WriteAt(content, content_size, 0) != (ssize_t) content_size )
	{
		Truncate(0);
		data_block->BeginWrite();
		memcpy(block_data, content, content_size);
		data->i_size = content_size;
		data->i_blocks = 0;
		data_block->FinishWrite();
		return false;
	}
	return true;
}

bool Inode::Rename(Inode* olddir, const char* oldname, const char* newname)
{
	if ( !strcmp(oldname, ".") || !strcmp(oldname, "..") ||
	     !strcmp(newname, ".") || !strcmp(newname, "..") )
		return errno = EPERM, false;
	Inode* src_inode = olddir->Open(oldname, O_RDONLY, 0);
	if ( !src_inode )
		return false;
	if ( Inode* dst_inode = Open(newname, O_RDONLY, 0) )
	{
		bool same_inode = src_inode->inode_id == dst_inode->inode_id;
		dst_inode->Unref();
		if ( same_inode )
			return src_inode->Unref(), true;
	}
	// TODO: Prove that this cannot fail and handle such a situation.
	if ( EXT2_S_ISDIR(src_inode->Mode()) )
	{
		if ( !Unlink(newname, true) && errno != ENOENT )
			return src_inode->Unref(), false;
		Link(newname, src_inode, true);
		olddir->Unlink(oldname, true, true);
		if ( olddir != this )
		{
			src_inode->Unlink("..", true, true);
			src_inode->Link("..", this, true);
		}
	}
	else
	{
		if ( !Unlink(newname, false) && errno != ENOENT )
			return src_inode->Unref(), false;
		Link(newname, src_inode, false);
		olddir->Unlink(oldname, false);
	}

	src_inode->Unref();
	return true;
}

bool Inode::Symlink(const char* elem, const char* dest)
{
	if ( !filesystem->device->write )
		return errno = EROFS, false;

	// TODO: Preferred block group!
	uint32_t result_inode_id = filesystem->AllocateInode();
	if ( !result_inode_id )
		return NULL;

	Inode* result = filesystem->GetInode(result_inode_id);
	if ( !result )
		return filesystem->FreeInode(result_inode_id), false;
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	result->BeginWrite();
	memset(result->data, 0, sizeof(*result->data));
	result->data->i_atime = now.tv_sec;
	result->data->i_ctime = now.tv_sec;
	result->data->i_mtime = now.tv_sec;
	// TODO: Set all the other inode properties!
	result->FinishWrite();
	result->SetMode((0777 & S_SETABLE) | EXT2_S_IFLNK);
	result->SetUserId(request_uid);
	result->SetGroupId(request_gid);

	size_t dest_length = strlen(dest);
	if ( SSIZE_MAX < dest_length )
		return errno = EFBIG, -1;
	if ( result->WriteAt((const uint8_t*) dest, dest_length, 0) < (ssize_t) dest_length )
		return result->Unref(), false;

	if ( !Link(elem, result, false) )
		return result->Truncate(0), result->Unref(), false;

	result->Unref();
	return true;
}

Inode* Inode::CreateDirectory(const char* path, mode_t mode)
{
	if ( !filesystem->device->write )
		return errno = EROFS, (Inode*) NULL;

	// TODO: Preferred block group!
	uint32_t result_inode_id = filesystem->AllocateInode();
	if ( !result_inode_id )
		return NULL;

	Inode* result = filesystem->GetInode(result_inode_id);
	if ( !result )
		return filesystem->FreeInode(result_inode_id), (Inode*) NULL;
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	result->BeginWrite();
	memset(result->data, 0, sizeof(*result->data));
	result->data->i_atime = now.tv_sec;
	result->data->i_ctime = now.tv_sec;
	result->data->i_mtime = now.tv_sec;
	// TODO: Set all the other inode properties!
	result->FinishWrite();
	result->SetMode((mode & S_SETABLE) | EXT2_S_IFDIR);
	result->SetUserId(request_uid);
	result->SetGroupId(request_gid);

	// Increase the directory count statistics.
	uint32_t group_id = (result->inode_id - 1) / filesystem->sb->s_inodes_per_group;
	assert(group_id < filesystem->num_groups);
	BlockGroup* block_group = filesystem->GetBlockGroup(group_id);
	if ( !block_group )
		return result->Unref(), (Inode*) NULL;
	block_group->BeginWrite();
	block_group->data->bg_used_dirs_count++;
	block_group->FinishWrite();
	block_group->Unref();

	if ( !Link(path, result, true) )
		return result->Unref(), (Inode*) NULL;

	if ( !result->Link(".", result, true) )
	{
		Unlink(path, true, true);
		return result->Unref(), (Inode*) NULL;
	}

	if ( !result->Link("..", this, true) )
	{
		result->Unlink(".", true, true);
		Unlink(path, true, true);
		return result->Unref(), (Inode*) NULL;
	}

	return result;
}

bool Inode::RemoveDirectory(const char* path)
{
	Inode* result = UnlinkKeep(path, true);
	if ( !result )
		return false;
	result->Unlink("..", true, true);
	result->Unlink(".", true, true);
	result->Truncate(0);

	// Decrease the directory count statistics.
	uint32_t group_id = (result->inode_id - 1) / filesystem->sb->s_inodes_per_group;
	assert(group_id < filesystem->num_groups);
	BlockGroup* block_group = filesystem->GetBlockGroup(group_id);
	if ( block_group )
	{
		block_group->BeginWrite();
		block_group->data->bg_used_dirs_count--;
		block_group->FinishWrite();
		block_group->Unref();
	}

	result->Unref();

	return true;
}

bool Inode::IsEmptyDirectory()
{
	if ( !EXT2_S_ISDIR(Mode()) )
		return errno = ENOTDIR, false;
	uint32_t block_size = filesystem->block_size;
	uint64_t filesize = Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	while ( offset < filesize )
	{
		uint64_t entry_block_id = offset / block_size;
		uint64_t entry_block_offset = offset % block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = GetBlock(block_id = entry_block_id)) )
			return false;
		uint8_t* block_data = block->block_data + entry_block_offset;
		struct ext_dirent* entry = (struct ext_dirent*) block_data;
		if ( entry->inode &&
		     !((entry->name_len == 1 && entry->name[0] == '.') ||
		       (entry->name_len == 2 && entry->name[0] == '.' &&
		                                entry->name[1] == '.' )) )
		{
			block->Unref();
			return false;
		}
		offset += entry->reclen;
	}
	if ( block )
		block->Unref();
	return true;
}

void Inode::Delete()
{
	assert(!data->i_links_count);
	assert(!reference_count);
	assert(!remote_reference_count);

	Truncate(0);

	uint32_t deleted_inode_id = inode_id;

	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	BeginWrite();
	memset(data, 0, sizeof(*data));
	data->i_dtime = now.tv_sec;
	FinishWrite();

	filesystem->FreeInode(deleted_inode_id);
}

void Inode::Refer()
{
	reference_count++;
}

void Inode::Unref()
{
	assert(0 < reference_count);
	reference_count--;
	if ( !reference_count && !remote_reference_count )
	{
		if ( !data->i_links_count )
			Delete();
		delete this;
	}
}

void Inode::RemoteRefer()
{
	remote_reference_count++;
}

void Inode::RemoteUnref()
{
	assert(0 < remote_reference_count);
	remote_reference_count--;
	if ( !reference_count && !remote_reference_count )
	{
		if ( !data->i_links_count )
			Delete();
		delete this;
	}
}

void Inode::Modified()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	BeginWrite();
	data->i_mtime = now.tv_sec;
	FinishWrite();
}

void Inode::BeginWrite()
{
	data_block->BeginWrite();
}

void Inode::FinishWrite()
{
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	data->i_ctime = now.tv_sec;
	if ( !dirty )
	{
		dirty = true;
		prev_dirty = NULL;
		next_dirty = filesystem->dirty_inode;
		if ( next_dirty )
			next_dirty->prev_dirty = this;
		filesystem->dirty_inode = this;
	}
	data_block->FinishWrite();
	Use();
}

void Inode::Sync()
{
	if ( !dirty )
		return;
	data_block->Sync();
	// TODO: The inode contents needs to be sync'd as well!
	(prev_dirty ? prev_dirty->next_dirty : filesystem->dirty_inode) = next_dirty;
	if ( next_dirty )
		next_dirty->prev_dirty = prev_dirty;
	prev_dirty = NULL;
	next_dirty = NULL;
	dirty = false;
}

void Inode::Use()
{
	data_block->Use();
	Unlink();
	Prelink();
}

void Inode::Unlink()
{
	(prev_inode ? prev_inode->next_inode : filesystem->mru_inode) = next_inode;
	(next_inode ? next_inode->prev_inode : filesystem->lru_inode) = prev_inode;
	size_t bin = inode_id % INODE_HASH_LENGTH;
	(prev_hashed ? prev_hashed->next_hashed : filesystem->hash_inodes[bin]) = next_hashed;
	if ( next_hashed ) next_hashed->prev_hashed = prev_hashed;
}

void Inode::Prelink()
{
	prev_inode = NULL;
	next_inode = filesystem->mru_inode;
	if ( filesystem->mru_inode )
		filesystem->mru_inode->prev_inode = this;
	filesystem->mru_inode = this;
	if ( !filesystem->lru_inode )
		filesystem->lru_inode = this;
	size_t bin = inode_id % INODE_HASH_LENGTH;
	prev_hashed = NULL;
	next_hashed = filesystem->hash_inodes[bin];
	filesystem->hash_inodes[bin] = this;
	if ( next_hashed )
		next_hashed->prev_hashed = this;
}
