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

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__OFF_MAX) && !defined(OFF_MAX)
#define OFF_MAX __OFF_MAX
#endif

#if defined(__sortix__)
#include <sortix/dirent.h>
#endif

#if defined(__sortix__)
#include <ioleast.h>
#include <timespec.h>
#else
struct timespec timespec_make(time_t sec, long nsec)
{
	struct timespec ret;
	ret.tv_sec = sec;
	ret.tv_nsec = nsec;
	return ret;
}
#endif

#if defined(__linux__)
#define FUSE_USE_VERSION 26
#include <fuse.h>
#endif

#if defined(__sortix__)
#include <fsmarshall.h>
#endif

#include "ext-constants.h"
#include "ext-structs.h"

#include "blockgroup.h"
#include "block.h"
#include "device.h"
#include "filesystem.h"
#include "inode.h"
#include "ioleast.h"
#include "util.h"

static volatile bool should_terminate = false;

const uint32_t EXT2_FEATURE_COMPAT_SUPPORTED = 0;
const uint32_t EXT2_FEATURE_INCOMPAT_SUPPORTED = \
               EXT2_FEATURE_INCOMPAT_FILETYPE;
const uint32_t EXT2_FEATURE_RO_COMPAT_SUPPORTED = \
               EXT2_FEATURE_RO_COMPAT_LARGE_FILE;

// TODO: Inode 0 is not legal, but a lot of functions here accept it!

mode_t HostModeFromExtMode(uint32_t extmode)
{
	mode_t hostmode = extmode & 0777;
	if ( extmode & EXT2_S_ISVTX ) hostmode |= S_ISVTX;
#if defined(S_ISGID) // Not on Sortix.
	if ( extmode & EXT2_S_ISGID ) hostmode |= S_ISGID;
#endif
#if defined(S_ISUID) // Not on Sortix.
	if ( extmode & EXT2_S_ISUID ) hostmode |= S_ISUID;
#endif
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
#if defined(S_ISGID) // Not on Sortix.
	if ( hostmode & S_ISGID ) extmode |= EXT2_S_ISGID;
#endif
#if defined(S_ISUID) // Not on Sortix.
	if ( hostmode & EXT2_S_ISUID ) extmode |= EXT2_S_ISUID;
#endif
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
	st->st_atim = timespec_make(inode->data->i_atime, 0);
	st->st_ctim = timespec_make(inode->data->i_ctime, 0);
	st->st_mtim = timespec_make(inode->data->i_mtime, 0);
	st->st_blksize = inode->filesystem->block_size;
	st->st_blocks = inode->data->i_blocks;
}

#if defined(__sortix__)

bool RespondData(int chl, const void* ptr, size_t count)
{
	return writeall(chl, ptr, count) == count;
}

bool RespondHeader(int chl, size_t type, size_t size)
{
	struct fsm_msg_header hdr;
	hdr.msgtype = type;
	hdr.msgsize = size;
	return RespondData(chl, &hdr, sizeof(hdr));
}

bool RespondMessage(int chl, unsigned int type, const void* ptr, size_t count)
{
	return RespondHeader(chl, type, count) &&
	       RespondData(chl, ptr, count);
}

bool RespondError(int chl, int errnum)
{
	struct fsm_resp_error body;
	body.errnum = errnum;
	//fprintf(stderr, "extfs: sending error %i (%s)\n", errnum, strerror(errnum));
	return RespondMessage(chl, FSM_RESP_ERROR, &body, sizeof(body));
}

bool RespondSuccess(int chl)
{
	struct fsm_resp_success body;
	return RespondMessage(chl, FSM_RESP_SUCCESS, &body, sizeof(body));
}

bool RespondStat(int chl, struct stat* st)
{
	struct fsm_resp_stat body;
	body.st = *st;
	return RespondMessage(chl, FSM_RESP_STAT, &body, sizeof(body));
}

bool RespondSeek(int chl, off_t offset)
{
	struct fsm_resp_lseek body;
	body.offset = offset;
	return RespondMessage(chl, FSM_RESP_LSEEK, &body, sizeof(body));
}

bool RespondRead(int chl, const uint8_t* buf, size_t count)
{
	struct fsm_resp_read body;
	body.count = count;
	return RespondMessage(chl, FSM_RESP_READ, &body, sizeof(body)) &&
	       RespondData(chl, buf, count);
}

bool RespondReadlink(int chl, const uint8_t* buf, size_t count)
{
	struct fsm_resp_readlink body;
	body.targetlen = count;
	return RespondMessage(chl, FSM_RESP_READLINK, &body, sizeof(body)) &&
	       RespondData(chl, buf, count);
}

bool RespondWrite(int chl, size_t count)
{
	struct fsm_resp_write body;
	body.count = count;
	return RespondMessage(chl, FSM_RESP_WRITE, &body, sizeof(body));
}

bool RespondOpen(int chl, ino_t ino, mode_t type)
{
	struct fsm_resp_open body;
	body.ino = ino;
	body.type = type;
	return RespondMessage(chl, FSM_RESP_OPEN, &body, sizeof(body));
}

bool RespondMakeDir(int chl, ino_t ino)
{
	struct fsm_resp_mkdir body;
	body.ino = ino;
	return RespondMessage(chl, FSM_RESP_MKDIR, &body, sizeof(body));
}

bool RespondReadDir(int chl, struct kernel_dirent* dirent)
{
	struct fsm_resp_readdirents body;
	body.ino = dirent->d_ino;
	body.type = dirent->d_type;
	body.namelen = dirent->d_namlen;
	return RespondMessage(chl, FSM_RESP_READDIRENTS, &body, sizeof(body)) &&
	       RespondData(chl, dirent->d_name, dirent->d_namlen);
}

void HandleRefer(int chl, struct fsm_req_refer* msg, Filesystem* fs)
{
	(void) chl;
	if ( fs->num_inodes <= msg->ino )
		return;
	if ( Inode* inode = fs->GetInode((uint32_t) msg->ino) )
		inode->RemoteRefer();
}

void HandleUnref(int chl, struct fsm_req_unref* msg, Filesystem* fs)
{
	(void) chl;
	if ( fs->num_inodes <= msg->ino )
		return;
	if ( Inode* inode = fs->GetInode((uint32_t) msg->ino) )
		inode->RemoteUnref();
}

void HandleSync(int chl, struct fsm_req_sync* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->Sync();
	inode->Unref();
	RespondSuccess(chl);
}

void HandleStat(int chl, struct fsm_req_stat* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	struct stat st;
	StatInode(inode, &st);
	inode->Unref();
	RespondStat(chl, &st);
}

void HandleChangeMode(int chl, struct fsm_req_chmod* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	uint32_t req_mode = ExtModeFromHostMode(msg->mode);
	uint32_t old_mode = inode->Mode();
	uint32_t new_mode = (old_mode & ~S_SETABLE) | (req_mode & S_SETABLE);
	inode->SetMode(new_mode);
	inode->Unref();
	RespondSuccess(chl);
}

void HandleChangeOwner(int chl, struct fsm_req_chown* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->SetUserId((uint32_t) msg->uid);
	inode->SetGroupId((uint32_t) msg->gid);
	inode->Unref();
	RespondSuccess(chl);
}

void HandleUTimens(int chl, struct fsm_req_utimens* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->data->i_atime = msg->times[0].tv_sec;
	inode->data->i_mtime = msg->times[1].tv_sec;
	inode->Dirty();
	inode->Unref();
	RespondSuccess(chl);
}

void HandleTruncate(int chl, struct fsm_req_truncate* msg, Filesystem* fs)
{
	if( msg->size < 0 ) { RespondError(chl, EINVAL); return; }
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->Truncate((uint64_t) msg->size);
	inode->Unref();
	RespondSuccess(chl);
}

void HandleSeek(int chl, struct fsm_req_lseek* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( msg->whence == SEEK_SET )
		RespondSeek(chl, msg->offset);
	else if ( msg->whence == SEEK_END )
	{
		off_t inode_size = inode->Size();
		if ( (msg->offset < 0 && inode_size + msg->offset < 0) ||
		     (0 <= msg->offset && OFF_MAX - inode_size < msg->offset) )
			RespondError(chl, EOVERFLOW);
		else
			RespondSeek(chl, msg->offset + inode_size);
	}
	else
		RespondError(chl, EINVAL);
	inode->Unref();
}

void HandleReadAt(int chl, struct fsm_req_pread* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	uint8_t* buf = (uint8_t*) malloc(msg->count);
	if ( !buf ) { inode->Unref(); RespondError(chl, errno); return; }
	ssize_t amount = inode->ReadAt(buf, msg->count, msg->offset);
	RespondRead(chl, buf, amount);
	inode->Unref();
	free(buf);
}

void HandleWriteAt(int chl, struct fsm_req_pread* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	const uint8_t* buf = (const uint8_t*) &msg[1];
	ssize_t amount = inode->WriteAt(buf, msg->count, msg->offset);
	RespondWrite(chl, amount);
	inode->Unref();
}

void HandleOpen(int chl, struct fsm_req_open* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->namelen+1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, pathraw, msg->namelen);
	path[msg->namelen] = '\0';

	Inode* result = inode->Open(path, msg->flags, ExtModeFromHostMode(msg->mode));

	free(path);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondOpen(chl, result->inode_id, result->Mode() & S_IFMT);
	result->Unref();
}

void HandleMakeDir(int chl, struct fsm_req_open* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->namelen+1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, pathraw, msg->namelen);
	path[msg->namelen] = '\0';

	Inode* result = inode->CreateDirectory(path, ExtModeFromHostMode(msg->mode));

	free(path);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondMakeDir(chl, result->inode_id);
	result->Unref();
}

void HandleReadDir(int chl, struct fsm_req_readdirents* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( !S_ISDIR(inode->Mode()) )
	{
		inode->Unref();
		RespondError(chl, ENOTDIR);
		return;
	}
	union
	{
		struct kernel_dirent kernel_entry;
		uint8_t padding[sizeof(struct kernel_dirent) + 256];
	};
	memset(&kernel_entry, 0, sizeof(kernel_entry));

	uint64_t file_size = inode->Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	while ( offset < file_size )
	{
		uint64_t entry_block_id = offset / fs->block_size;
		uint64_t entry_block_offset = offset % fs->block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = inode->GetBlock(block_id = entry_block_id)) )
		{
			inode->Unref();
			RespondError(chl, errno);
			return;
		}
		const uint8_t* block_data = block->block_data + entry_block_offset;
		const struct ext_dirent* entry = (const struct ext_dirent*) block_data;
		if ( entry->inode && entry->name_len && !(msg->rec_num--) )
		{
			kernel_entry.d_reclen = sizeof(kernel_entry) + entry->name_len;
			kernel_entry.d_nextoff = 0;
			kernel_entry.d_ino = entry->inode;
			kernel_entry.d_dev = 0;
			kernel_entry.d_type = 0; // TODO: Support this!
			kernel_entry.d_namlen = entry->name_len;
			memcpy(kernel_entry.d_name, entry->name, entry->name_len);
			size_t dname_offset = offsetof(struct kernel_dirent, d_name);
			padding[dname_offset + kernel_entry.d_namlen] = '\0';
			block->Unref();
			inode->Unref();
			RespondReadDir(chl, &kernel_entry);
			return;
		}
		offset += entry->reclen;
	}
	if ( block )
		block->Unref();

	kernel_entry.d_reclen = sizeof(kernel_entry);
	RespondReadDir(chl, &kernel_entry);
}

void HandleIsATTY(int chl, struct fsm_req_isatty* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	RespondError(chl, ENOTTY);
	inode->Unref();
}

void HandleUnlink(int chl, struct fsm_req_unlink* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->namelen+1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, pathraw, msg->namelen);
	path[msg->namelen] = '\0';

	Inode* result = inode->Unlink(path, false);
	free(path);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	result->Unref();

	RespondSuccess(chl);
}

void HandleRemoveDir(int chl, struct fsm_req_unlink* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->namelen+1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, pathraw, msg->namelen);
	path[msg->namelen] = '\0';

	if ( inode->RemoveDirectory(path) )
		RespondSuccess(chl);
	else
		RespondError(chl, errno);

	free(path);
	inode->Unref();
}

void HandleLink(int chl, struct fsm_req_link* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	if ( fs->num_inodes <= msg->linkino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }
	Inode* dest = fs->GetInode((uint32_t) msg->linkino);
	if ( !dest ) { inode->Unref(); RespondError(chl, errno); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->namelen+1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, pathraw, msg->namelen);
	path[msg->namelen] = '\0';

	if ( inode->Link(path, dest, false) )
		RespondSuccess(chl);
	else
		RespondError(chl, errno);

	free(path);
	dest->Unref();
	inode->Unref();
}

void HandleSymlink(int chl, struct fsm_req_symlink* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->dirino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }

	char* dest_raw = (char*) &(msg[1]);
	char* dest = (char*) malloc(msg->targetlen + 1);
	if ( !dest )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(dest, dest_raw, msg->targetlen);
	dest[msg->targetlen] = '\0';

	char* path_raw = (char*) dest_raw + msg->targetlen;
	char* path = (char*) malloc(msg->namelen + 1);
	if ( !path )
	{
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, path_raw, msg->namelen);
	path[msg->namelen] = '\0';

	if ( inode->Symlink(path, dest) )
		RespondSuccess(chl);
	else
		RespondError(chl, errno);

	free(path);
	free(dest);
	inode->Unref();
}

void HandleReadlink(int chl, struct fsm_req_readlink* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->ino ) { RespondError(chl, EBADF); return; }
	Inode* inode = fs->GetInode((uint32_t) msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( !EXT2_S_ISLNK(inode->Mode()) ) { RespondError(chl, EINVAL); return; }
	size_t count = inode->Size();
	uint8_t* buf = (uint8_t*) malloc(count);
	if ( !buf ) { inode->Unref(); RespondError(chl, errno); return; }
	ssize_t amount = inode->ReadAt(buf, count, 0);
	RespondReadlink(chl, buf, amount);
	inode->Unref();
	free(buf);
}

void HandleRename(int chl, struct fsm_req_rename* msg, Filesystem* fs)
{
	if ( fs->num_inodes <= msg->olddirino ) { RespondError(chl, EBADF); return; }
	if ( fs->num_inodes <= msg->newdirino ) { RespondError(chl, EBADF); return; }

	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->oldnamelen+1 + msg->newnamelen+1);
	if ( !path ) { RespondError(chl, errno); return; }
	memcpy(path, pathraw, msg->oldnamelen);
	path[msg->oldnamelen] = '\0';
	memcpy(path + msg->oldnamelen + 1, pathraw + msg->oldnamelen, msg->newnamelen);
	path[msg->oldnamelen + 1 + msg->newnamelen] = '\0';

	const char* oldname = path;
	const char* newname = path + msg->oldnamelen + 1;

	Inode* olddir = fs->GetInode((uint32_t) msg->olddirino);
	if ( !olddir ) { free(path); RespondError(chl, errno); return; }
	Inode* newdir = fs->GetInode((uint32_t) msg->newdirino);
	if ( !newdir ) { olddir->Unref(); free(path); RespondError(chl, errno); return; }

	if ( newdir->Rename(olddir, oldname, newname) )
		RespondSuccess(chl);
	else
		RespondError(chl, errno);

	newdir->Unref();
	olddir->Unref();
	free(path);
}

void HandleIncomingMessage(int chl, struct fsm_msg_header* hdr, Filesystem* fs)
{
	typedef void (*handler_t)(int, void*, Filesystem*);
	handler_t handlers[FSM_MSG_NUM] = { NULL };
	handlers[FSM_REQ_SYNC] = (handler_t) HandleSync;
	handlers[FSM_REQ_STAT] = (handler_t) HandleStat;
	handlers[FSM_REQ_CHMOD] = (handler_t) HandleChangeMode;
	handlers[FSM_REQ_CHOWN] = (handler_t) HandleChangeOwner;
	handlers[FSM_REQ_TRUNCATE] = (handler_t) HandleTruncate;
	handlers[FSM_REQ_LSEEK] = (handler_t) HandleSeek;
	handlers[FSM_REQ_PREAD] = (handler_t) HandleReadAt;
	handlers[FSM_REQ_OPEN] = (handler_t) HandleOpen;
	handlers[FSM_REQ_READDIRENTS] = (handler_t) HandleReadDir;
	handlers[FSM_REQ_PWRITE] = (handler_t) HandleWriteAt;
	handlers[FSM_REQ_ISATTY] = (handler_t) HandleIsATTY;
	handlers[FSM_REQ_UTIMENS] = (handler_t) HandleUTimens;
	handlers[FSM_REQ_MKDIR] = (handler_t) HandleMakeDir;
	handlers[FSM_REQ_RMDIR] = (handler_t) HandleRemoveDir;
	handlers[FSM_REQ_UNLINK] = (handler_t) HandleUnlink;
	handlers[FSM_REQ_LINK] = (handler_t) HandleLink;
	handlers[FSM_REQ_SYMLINK] = (handler_t) HandleSymlink;
	handlers[FSM_REQ_READLINK] = (handler_t) HandleReadlink;
	handlers[FSM_REQ_RENAME] = (handler_t) HandleRename;
	// TODO: symlink
	// TODO: readlink
	handlers[FSM_REQ_REFER] = (handler_t) HandleRefer;
	handlers[FSM_REQ_UNREF] = (handler_t) HandleUnref;
	if ( FSM_MSG_NUM <= hdr->msgtype || !handlers[hdr->msgtype] )
	{
		fprintf(stderr, "extfs: message type %zu not supported!\n", hdr->msgtype);
		RespondError(chl, ENOTSUP);
		return;
	}
	uint8_t* body = (uint8_t*) malloc(hdr->msgsize);
	if ( !body )
	{
		fprintf(stderr, "extfs: message of type %zu too large: %zu bytes\n", hdr->msgtype, hdr->msgsize);
		RespondError(chl, errno);
		return;
	}
	size_t amount = readall(chl, body, hdr->msgsize);
	if ( amount < hdr->msgsize )
	{
		fprintf(stderr, "extfs: incomplete message of type %zu: got %zi of %zu bytes\n", hdr->msgtype, amount, hdr->msgsize);
		RespondError(chl, errno);
		free(body);
		return;
	}
	handlers[hdr->msgtype](chl, body, fs);
	free(body);
}

void AlarmHandler(int)
{
}

void TerminationHandler(int)
{
	should_terminate = true;
}

#endif

#if defined(__linux__)

struct ext2_fuse_ctx
{
	Device* dev;
	Filesystem* fs;
};

#ifndef S_SETABLE
#define S_SETABLE 02777
#endif

#define FUSE_FS (((struct ext2_fuse_ctx*) (fuse_get_context()->private_data))->fs)

void* ext2_fuse_init(struct fuse_conn_info* /*conn*/)
{
	return fuse_get_context()->private_data;
}

void ext2_fuse_destroy(void* fs_private)
{
	struct ext2_fuse_ctx* ext2_fuse_ctx = (struct ext2_fuse_ctx*) fs_private;
	ext2_fuse_ctx->fs->Sync();
	ext2_fuse_ctx->dev->Sync();
	delete ext2_fuse_ctx->fs; ext2_fuse_ctx->fs = NULL;
	delete ext2_fuse_ctx->dev; ext2_fuse_ctx->dev = NULL;
}

Inode* ext2_fuse_resolve_path(const char* path)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode(EXT2_ROOT_INO);
	assert(inode);
	while ( path[0] )
	{
		if ( *path == '/' )
		{
			if ( !EXT2_S_ISDIR(inode->Mode()) )
				return errno = ENOTDIR, (Inode*) NULL;
			path++;
			continue;
		}
		size_t elem_len = strcspn(path, "/");
		char* elem = new char[elem_len+1];
		memcpy(elem, path, elem_len);
		elem[elem_len] = '\0';
		path += elem_len;
		Inode* next = inode->Open(elem, O_RDONLY, 0);
		delete[] elem;
		inode->Unref();
		if ( !next )
			return NULL;
		inode = next;
	}
	return inode;
}

// Assumes that the path doesn't end with / unless it's the root directory.
Inode* ext2_fuse_parent_dir(const char** path_ptr)
{
	const char* path = *path_ptr;
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode(EXT2_ROOT_INO);
	assert(inode);
	while ( strchr(path, '/') )
	{
		if ( *path == '/' )
		{
			if ( !EXT2_S_ISDIR(inode->Mode()) )
				return errno = ENOTDIR, (Inode*) NULL;
			path++;
			continue;
		}
		size_t elem_len = strcspn(path, "/");
		char* elem = new char[elem_len+1];
		memcpy(elem, path, elem_len);
		elem[elem_len] = '\0';
		path += elem_len;
		Inode* next = inode->Open(elem, O_RDONLY, 0);
		delete[] elem;
		inode->Unref();
		if ( !next )
			return NULL;
		inode = next;
	}
	*path_ptr = *path ? path : ".";
	assert(!strchr(*path_ptr, '/'));
	return inode;
}

int ext2_fuse_getattr(const char* path, struct stat* st)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	StatInode(inode, st);
	inode->Unref();
	return 0;
}

int ext2_fuse_fgetattr(const char* /*path*/, struct stat* st,
                        struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	StatInode(inode, st);
	inode->Unref();
	return 0;
}

int ext2_fuse_readlink(const char* path, char* buf, size_t bufsize)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	if ( !EXT2_S_ISLNK(inode->Mode()) )
		return inode->Unref(), -(errno = EINVAL);
	if ( !bufsize )
		return inode->Unref(), -(errno = EINVAL);
	ssize_t amount = inode->ReadAt((uint8_t*) buf, bufsize, 0);
	if ( amount < 0 )
		return inode->Unref(), -errno;
	buf[(size_t) amount < bufsize ? (size_t) bufsize : bufsize - 1] = '\0';
	inode->Unref();
	return 0;
}

int ext2_fuse_mknod(const char* path, mode_t mode, dev_t dev)
{
	(void) path;
	(void) mode;
	(void) dev;
	return -(errno = ENOSYS);
}

int ext2_fuse_mkdir(const char* path, mode_t mode)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	Inode* newdir = inode->CreateDirectory(path, ExtModeFromHostMode(mode));
	inode->Unref();
	if ( !newdir )
		return -errno;
	newdir->Unref();
	return 0;
}

int ext2_fuse_unlink(const char* path)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	Inode* result = inode->Unlink(path, false);
	inode->Unref();
	if ( !result )
		return -errno;
	result->Unref();
	return 0;
}

int ext2_fuse_rmdir(const char* path)
{
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	bool success = inode->RemoveDirectory(path);
	inode->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_symlink(const char* oldname, const char* newname)
{
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return -errno;
	bool success = newdir->Symlink(newname, oldname);
	newdir->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_rename(const char* oldname, const char* newname)
{
	Inode* olddir = ext2_fuse_parent_dir(&oldname);
	if ( !olddir )
		return -errno;
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return olddir->Unref(), -errno;
	bool success = newdir->Rename(olddir, oldname, newname);
	newdir->Unref();
	olddir->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_link(const char* oldname, const char* newname)
{
	Inode* inode = ext2_fuse_resolve_path(oldname);
	if ( !inode )
		return -errno;
	Inode* newdir = ext2_fuse_parent_dir(&newname);
	if ( !newdir )
		return inode->Unref(), -errno;
	bool success = inode->Link(newname, inode, false);
	newdir->Unref();
	inode->Unref();
	return success ? 0 : -errno;
}

int ext2_fuse_chmod(const char* path, mode_t mode)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	uint32_t req_mode = ExtModeFromHostMode(mode);
	uint32_t old_mode = inode->Mode();
	uint32_t new_mode = (old_mode & ~S_SETABLE) | (req_mode & S_SETABLE);
	inode->SetMode(new_mode);
	inode->Unref();
	return 0;
}

int ext2_fuse_chown(const char* path, uid_t owner, gid_t group)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	inode->SetUserId((uint32_t) owner);
	inode->SetGroupId((uint32_t) group);
	inode->Unref();
	return 0;
}

int ext2_fuse_truncate(const char* path, off_t size)
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	inode->Truncate((uint64_t) size);
	inode->Unref();
	return 0;
}

int ext2_fuse_ftruncate(const char* /*path*/, off_t size,
                        struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->Truncate((uint64_t) size);
	inode->Unref();
	return 0;
}

int ext2_fuse_open(const char* path, struct fuse_file_info* fi)
{
	int flags = fi->flags;
	Inode* dir = ext2_fuse_parent_dir(&path);
	if ( !dir )
		return -errno;
	Inode* result = dir->Open(path, flags, 0);
	dir->Unref();
	if ( !result )
		return -errno;
	fi->fh = (uint64_t) result->inode_id;
	fi->keep_cache = 1;
	result->RemoteRefer();
	result->Unref();
	return 0;
}

int ext2_fuse_access(const char* path, int mode)
{
	Inode* dir = ext2_fuse_parent_dir(&path);
	if ( !dir )
		return -errno;
	Inode* result = dir->Open(path, O_RDONLY, 0);
	dir->Unref();
	if ( !result )
		return -errno;
	(void) mode;
	result->Unref();
	return 0;
}

int ext2_fuse_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	int flags = fi->flags | O_CREAT;
	Inode* inode = ext2_fuse_parent_dir(&path);
	if ( !inode )
		return -errno;
	Inode* result = inode->Open(path, flags, ExtModeFromHostMode(mode));
	inode->Unref();
	if ( !result )
		return -errno;
	fi->fh = (uint64_t) result->inode_id;
	fi->keep_cache = 1;
	result->Unref();
	return 0;
}

int ext2_fuse_opendir(const char* path, struct fuse_file_info* fi)
{
	return ext2_fuse_open(path, fi);
}

int ext2_fuse_read(const char* /*path*/, char* buf, size_t count, off_t offset,
                   struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	if ( INT_MAX < count )
		count = INT_MAX;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	ssize_t result = inode->ReadAt((uint8_t*) buf, count, offset);
	inode->Unref();
	return 0 <= result ? (int) result : -errno;
}

int ext2_fuse_write(const char* /*path*/, const char* buf, size_t count,
                    off_t offset, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	if ( INT_MAX < count )
		count = INT_MAX;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	ssize_t result = inode->WriteAt((const uint8_t*) buf, count, offset);
	inode->Unref();
	return 0 <= result ? (int) result : -errno;
}

int ext2_fuse_statfs(const char* /*path*/, struct statvfs* stvfs)
{
	(void) stvfs;
	return errno = -ENOSYS, -1;
}

int ext2_fuse_flush(const char* /*path*/, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->Sync();
	inode->Unref();
	return 0;
}

int ext2_fuse_release(const char* /*path*/, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->RemoteUnref();
	inode->Unref();
	return 0;
}

int ext2_fuse_releasedir(const char* path, struct fuse_file_info* fi)
{
	return ext2_fuse_release(path, fi);
}

int ext2_fuse_fsync(const char* /*path*/, int data, struct fuse_file_info* fi)
{
	(void) data;
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	inode->Sync();
	inode->Unref();
	return 0;
}

/*int ext2_fuse_syncdir(const char* path, int data, struct fuse_file_info* fi)
{
	return ext2_fuse_sync(path, data, fi);
}*/

/*int ext2_fuse_setxattr(const char *, const char *, const char *, size_t, int)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_getxattr(const char *, const char *, char *, size_t)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_listxattr(const char *, char *, size_t)
{
	return -(errno = ENOSYS);
}*/

/*int ext2_fuse_removexattr(const char *, const char *)
{
	return -(errno = ENOSYS);
}*/

int ext2_fuse_readdir(const char* /*path*/, void* buf, fuse_fill_dir_t filler,
                      off_t rec_num, struct fuse_file_info* fi)
{
	Filesystem* fs = FUSE_FS;
	Inode* inode = fs->GetInode((uint32_t) fi->fh);
	if ( !inode )
		return -errno;
	if ( !S_ISDIR(inode->Mode()) )
		return inode->Unref(), -(errno = ENOTDIR);

	uint64_t file_size = inode->Size();
	uint64_t offset = 0;
	Block* block = NULL;
	uint64_t block_id = 0;
	while ( offset < file_size )
	{
		uint64_t entry_block_id = offset / fs->block_size;
		uint64_t entry_block_offset = offset % fs->block_size;
		if ( block && block_id != entry_block_id )
			block->Unref(),
			block = NULL;
		if ( !block && !(block = inode->GetBlock(block_id = entry_block_id)) )
			return inode->Unref(), -errno;
		const uint8_t* block_data = block->block_data + entry_block_offset;
		const struct ext_dirent* entry = (const struct ext_dirent*) block_data;
		if ( entry->inode && entry->name_len && (!rec_num || !rec_num--) )
		{
			char* entry_name = new char[entry->name_len+1];
			memcpy(entry_name, entry->name, entry->name_len);
			entry_name[entry->name_len] = '\0';
			bool full = filler(buf, entry_name, NULL, 0);
			delete[] entry_name;
			if ( full )
			{
				block->Unref();
				inode->Unref();
				return 0;
			}
		}
		offset += entry->reclen;
	}
	if ( block )
		block->Unref();

	inode->Sync();
	inode->Unref();
	return 0;
}

/*int ext2_fuse_lock(const char*, struct fuse_file_info*, int, struct flock*)
{
	return -(errno = ENOSYS);
}*/

int ext2_fuse_utimens(const char* path, const struct timespec tv[2])
{
	Inode* inode = ext2_fuse_resolve_path(path);
	if ( !inode )
		return -errno;
	inode->data->i_atime = tv[0].tv_sec;
	inode->data->i_mtime = tv[1].tv_sec;
	inode->Dirty();
	inode->Unref();
	return 0;
}

/*int ext2_fuse_bmap(const char*, size_t blocksize, uint64_t* idx)
{
	return -(errno = ENOSYS);
}*/

#endif

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

void compact_arguments(int* argc, char*** argv)
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

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [--probe] [--test-uuid UUID] DEVICE [MOUNT-POINT]\n", argv0);
}

void version(FILE* fp, const char* argv0)
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
		if ( arg[0] != '-' )
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
		else if ( !strcmp(arg, "--help") ) { help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { version(stdout, argv0); exit(0); }
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
	const char* mount_path = 2 <= argc ? argv[2] : NULL;

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
	if ( probe && sb.s_rev_level == EXT2_GOOD_OLD_REV )
		exit(1);

	if ( !probe && sb.s_rev_level == EXT2_GOOD_OLD_REV )
		error(1, 0, "`%s' is formatted with an obsolete filesystem revision",
		            device_path);

	// Verify that no incompatible features are in use.
	if ( probe && sb.s_feature_compat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED )
		exit(1);

	if ( !probe && sb.s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED )
		error(1, 0, "`%s' uses unsupported and incompatible features",
		            device_path);

	// Verify that no incompatible features are in use if opening for write.
	if ( probe && default_access && write &&
	     sb.s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED )
		exit(1);

	if ( !probe && default_access && write &&
	     sb.s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED )
			error(1, 0, "`%s uses unsupported and incompatible features, "
			            "read-only access is possible, but write-access was "
						"requested", device_path);

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

	// We have found no critical problems, so let the caller know that this
	// filesystem satisfies the probe request.
	if ( probe )
		exit(0);

	// Check whether the filesystem was unmounted cleanly.
	if ( !probe && sb.s_state != EXT2_VALID_FS )
		fprintf(stderr, "Warning: `%s' wasn't unmounted cleanly\n",
	                    device_path);

	uint32_t block_size = 1024U << sb.s_log_block_size;

	Device* dev = new Device(fd, block_size, write);
	Filesystem* fs = new Filesystem(dev);

	fs->block_groups = new BlockGroup*[fs->num_groups];
	for ( size_t i = 0; i < fs->num_groups; i++ )
		fs->block_groups[i] = NULL;

	if ( !mount_path )
		return 0;

#if defined(__sortix__)
	// Stat the root inode.
	struct stat root_inode_st;
	Inode* root_inode = fs->GetInode((uint32_t) EXT2_ROOT_INO);
	if ( !root_inode )
		error(1, errno, "GetInode(%u)", EXT2_ROOT_INO);
	StatInode(root_inode, &root_inode_st);
	root_inode->Unref();

	// Create a filesystem server connected to the kernel that we'll listen on.
	int serverfd = fsm_mountat(AT_FDCWD, mount_path, &root_inode_st, 0);
	if ( serverfd < 0 )
		error(1, errno, "%s", mount_path);

	// Make sure the server isn't unexpectedly killed and data is lost.
	signal(SIGINT, TerminationHandler);
	signal(SIGTERM, TerminationHandler);
	signal(SIGQUIT, TerminationHandler);

	// Become a background process in its own process group by default.
	if ( !foreground )
	{
		pid_t child_pid = fork();
		if ( child_pid < 0 )
			error(1, errno, "fork");
		if ( child_pid )
			exit(0);
		setpgid(0, 0);
	}

	// Listen for filesystem messages and sync the filesystem every few seconds.
	struct timespec last_sync_at;
	clock_gettime(CLOCK_MONOTONIC, &last_sync_at);
	int channel;
	while ( 0 <= (channel = accept(serverfd, NULL, NULL)) )
	{
		if ( should_terminate )
			break;
		struct fsm_msg_header hdr;
		size_t amount;
		if ( (amount = readall(channel, &hdr, sizeof(hdr))) != sizeof(hdr) )
		{
			error(0, errno, "incomplete header: got %zi of %zu bytes", amount, sizeof(hdr));
			errno = 0;
			continue;
		}
		HandleIncomingMessage(channel, &hdr, fs);
		close(channel);

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);

		if ( write && 5 <= timespec_sub(now, last_sync_at).tv_sec )
		{
			fs->Sync();
			last_sync_at = now;
		}
	}

	// Sync the filesystem before shutting down.
	if ( write )
	{
		fprintf(stderr, "%s: filesystem server shutting down, syncing...", argv0);
		fflush(stderr);
		fs->Sync();
		fprintf(stderr, " done.\n");
	}

	close(serverfd);

#elif defined(__linux__)

	(void) foreground;

	struct fuse_operations operations;
	memset(&operations, 0, sizeof(operations));

	operations.access = ext2_fuse_access;
	operations.chmod = ext2_fuse_chmod;
	operations.chown = ext2_fuse_chown;
	operations.create = ext2_fuse_create;
	operations.destroy = ext2_fuse_destroy;
	operations.fgetattr = ext2_fuse_fgetattr;
	operations.flush = ext2_fuse_flush;
	operations.fsync = ext2_fuse_fsync;
	operations.ftruncate = ext2_fuse_ftruncate;
	operations.getattr = ext2_fuse_getattr;
	operations.init = ext2_fuse_init;
	operations.link = ext2_fuse_link;
	operations.mkdir = ext2_fuse_mkdir;
	operations.mknod = ext2_fuse_mknod;
	operations.opendir = ext2_fuse_opendir;
	operations.open = ext2_fuse_open;
	operations.readdir = ext2_fuse_readdir;
	operations.read = ext2_fuse_read;
	operations.readlink = ext2_fuse_readlink;
	operations.releasedir = ext2_fuse_releasedir;
	operations.release = ext2_fuse_release;
	operations.rename = ext2_fuse_rename;
	operations.rmdir = ext2_fuse_rmdir;
	operations.statfs = ext2_fuse_statfs;
	operations.symlink = ext2_fuse_symlink;
	operations.truncate = ext2_fuse_truncate;
	operations.unlink = ext2_fuse_unlink;
	operations.utimens = ext2_fuse_utimens;
	operations.write = ext2_fuse_write;

	operations.flag_nullpath_ok = 1;
	operations.flag_nopath = 1;

	char* argv_fuse[] =
	{
		(char*) argv[0],
		(char*) "-s",
		(char*) mount_path,
		(char*) NULL,
	};

	int argc_fuse = sizeof(argv_fuse) / sizeof(argv_fuse[0]) - 1;

	struct ext2_fuse_ctx ext2_fuse_ctx;
	ext2_fuse_ctx.fs = fs;
	ext2_fuse_ctx.dev = dev;

	return fuse_main(argc_fuse, argv_fuse, &operations, &ext2_fuse_ctx);

#else

	(void) foreground;
	(void) mount_path;

#endif

	delete fs;
	delete dev;

	close(fd);

	return 0;
}
