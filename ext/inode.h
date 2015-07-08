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

    inode.h
    Filesystem inode.

*******************************************************************************/

#ifndef INODE_H
#define INODE_H

class Block;
class Filesystem;

class Inode
{
public:
	Inode(Filesystem* filesystem, uint32_t inode_id);
	~Inode();

public:
	Inode* prev_inode;
	Inode* next_inode;
	Inode* prev_hashed;
	Inode* next_hashed;
	Inode* prev_dirty;
	Inode* next_dirty;
	Block* data_block;
	struct ext_inode* data;
	Filesystem* filesystem;
	size_t reference_count;
	size_t remote_reference_count;
	uint32_t inode_id;
	bool dirty;

public:
	uint32_t Mode();
	uint32_t UserId();
	uint32_t GroupId();
	uint64_t Size();
	void SetMode(uint32_t mode);
	void SetUserId(uint32_t user);
	void SetGroupId(uint32_t group);
	void SetSize(uint64_t new_size);
	void Truncate(uint64_t new_size);
	bool FreeIndirect(uint64_t from, uint64_t offset, uint32_t block_id,
	                  int indirection, uint64_t entry_span);
	Block* GetBlock(uint64_t offset);
	Block* GetBlockFromTable(Block* table, uint32_t index);
	Inode* Open(const char* elem, int flags, mode_t mode);
	bool Link(const char* elem, Inode* dest, bool directories);
	bool Symlink(const char* elem, const char* dest);
	bool Unlink(const char* elem, bool directories, bool force=false);
	Inode* UnlinkKeep(const char* elem, bool directories, bool force=false);
	ssize_t ReadAt(uint8_t* buffer, size_t count, off_t offset);
	ssize_t WriteAt(const uint8_t* buffer, size_t count, off_t offset);
	bool UnembedInInode();
	bool Rename(Inode* olddir, const char* oldname, const char* newname);
	Inode* CreateDirectory(const char* path, mode_t mode);
	bool RemoveDirectory(const char* path);
	bool IsEmptyDirectory();
	void Refer();
	void Unref();
	void RemoteRefer();
	void RemoteUnref();
	void Sync();
	void BeginWrite();
	void FinishWrite();
	void Modified();
	void Use();
	void Unlink();
	void Prelink();
	void Linked();
	void Unlinked();
	void Delete();

};

#endif
