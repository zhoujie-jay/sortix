/*
 * Copyright (c) 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * fsmarshall.cpp
 * Sortix fsmarshall frontend.
 */

#if defined(__sortix__)

#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <error.h>
#include <dirent.h>
#include <fcntl.h>
#include <ioleast.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

#include <sortix/dirent.h>

#include <fsmarshall.h>

#include "ext-constants.h"
#include "ext-structs.h"

#include "blockgroup.h"
#include "block.h"
#include "device.h"
#include "extfs.h"
#include "filesystem.h"
#include "fsmarshall.h"
#include "fuse.h"
#include "inode.h"

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

bool RespondMessage(int chl, size_t type, const void* ptr, size_t count)
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

bool RespondStatVFS(int chl, struct statvfs* stvfs)
{
	struct fsm_resp_statvfs body;
	body.stvfs = *stvfs;
	return RespondMessage(chl, FSM_RESP_STATVFS, &body, sizeof(body));
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

bool RespondReadDir(int chl, struct dirent* dirent)
{
	struct fsm_resp_readdirents body;
	body.ino = dirent->d_ino;
	body.type = dirent->d_type;
	body.namelen = dirent->d_namlen;
	return RespondMessage(chl, FSM_RESP_READDIRENTS, &body, sizeof(body)) &&
	       RespondData(chl, dirent->d_name, dirent->d_namlen);
}

bool RespondTCGetBlob(int chl, const void* data, size_t data_size)
{
	struct fsm_resp_tcgetblob body;
	body.count = data_size;
	return RespondMessage(chl, FSM_RESP_TCGETBLOB, &body, sizeof(body)) &&
	       RespondData(chl, data, data_size);
}

Inode* SafeGetInode(Filesystem* fs, ino_t ino)
{
	if ( (uint32_t) ino != ino )
		return errno = EBADF, (Inode*) ino;
	// TODO: Should check if the inode is not deleted.
	return fs->GetInode((uint32_t) ino);
}

void HandleRefer(int chl, struct fsm_req_refer* msg, Filesystem* fs)
{
	(void) chl;
	if ( Inode* inode = SafeGetInode(fs, (uint32_t) msg->ino) )
	{
		inode->RemoteRefer();
		inode->Unref();
	}
}

void HandleUnref(int chl, struct fsm_req_unref* msg, Filesystem* fs)
{
	(void) chl;
	if ( Inode* inode = SafeGetInode(fs, (uint32_t) msg->ino) )
	{
		inode->RemoteUnref();
		inode->Unref();
	}
}

void HandleSync(int chl, struct fsm_req_sync* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->Sync();
	inode->Unref();
	RespondSuccess(chl);
}

void HandleStat(int chl, struct fsm_req_stat* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	struct stat st;
	StatInode(inode, &st);
	inode->Unref();
	RespondStat(chl, &st);
}

void HandleChangeMode(int chl, struct fsm_req_chmod* msg, Filesystem* fs)
{
	if ( !fs->device->write ) { RespondError(chl, EROFS); return; }
	Inode* inode = SafeGetInode(fs, msg->ino);
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
	if ( !fs->device->write ) { RespondError(chl, EROFS); return; }
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->SetUserId((uint32_t) msg->uid);
	inode->SetGroupId((uint32_t) msg->gid);
	inode->Unref();
	RespondSuccess(chl);
}

void HandleUTimens(int chl, struct fsm_req_utimens* msg, Filesystem* fs)
{
	if ( !fs->device->write ) { RespondError(chl, EROFS); return; }
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( msg->times[0].tv_nsec != UTIME_OMIT ||
	     msg->times[1].tv_nsec != UTIME_OMIT )
	{
		time_t now = time(NULL);
		inode->BeginWrite();
		if ( msg->times[0].tv_nsec == UTIME_NOW )
			inode->data->i_atime = now;
		else if ( msg->times[0].tv_nsec != UTIME_OMIT )
			inode->data->i_atime = msg->times[0].tv_sec;
		if ( msg->times[1].tv_nsec == UTIME_NOW )
			inode->data->i_mtime = now;
		else if ( msg->times[1].tv_nsec != UTIME_OMIT )
			inode->data->i_mtime = msg->times[1].tv_sec;
		inode->FinishWrite();
	}
	inode->Unref();
	RespondSuccess(chl);
}

void HandleTruncate(int chl, struct fsm_req_truncate* msg, Filesystem* fs)
{
	if ( !fs->device->write ) { RespondError(chl, EROFS); return; }
	if ( msg->size < 0 ) { RespondError(chl, EINVAL); return; }
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	inode->Truncate((uint64_t) msg->size);
	inode->Unref();
	RespondSuccess(chl);
}

void HandleSeek(int chl, struct fsm_req_lseek* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
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
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	uint8_t* buf = (uint8_t*) malloc(msg->count);
	if ( !buf ) { inode->Unref(); RespondError(chl, errno); return; }
	ssize_t amount = inode->ReadAt(buf, msg->count, msg->offset);
	inode->Unref();
	if ( amount < 0 ) { free(buf); RespondError(chl, errno); return; }
	RespondRead(chl, buf, amount);
	free(buf);
}

void HandleWriteAt(int chl, struct fsm_req_pwrite* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	const uint8_t* buf = (const uint8_t*) &msg[1];
	ssize_t amount = inode->WriteAt(buf, msg->count, msg->offset);
	inode->Unref();
	if ( amount < 0 ) { RespondError(chl, errno); return; }
	RespondWrite(chl, amount);
}

void HandleOpen(int chl, struct fsm_req_open* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
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

void HandleMakeDir(int chl, struct fsm_req_mkdir* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
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
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( !S_ISDIR(inode->Mode()) )
	{
		inode->Unref();
		RespondError(chl, ENOTDIR);
		return;
	}
	union
	{
		struct dirent kernel_entry;
		uint8_t padding[sizeof(struct dirent) + 256];
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
			uint8_t file_type = EXT2_FT_UNKNOWN;
			if ( fs->sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE )
				file_type = entry->file_type;
			kernel_entry.d_reclen = sizeof(kernel_entry) + entry->name_len;
			kernel_entry.d_ino = entry->inode;
			kernel_entry.d_dev = 0;
			kernel_entry.d_type = HostDTFromExtDT(file_type);
			kernel_entry.d_namlen = entry->name_len;
			memcpy(kernel_entry.d_name, entry->name, entry->name_len);
			size_t dname_offset = offsetof(struct dirent, d_name);
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
	inode->Unref();

	kernel_entry.d_reclen = sizeof(kernel_entry);
	RespondReadDir(chl, &kernel_entry);
}

void HandleIsATTY(int chl, struct fsm_req_isatty* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	RespondError(chl, ENOTTY);
	inode->Unref();
}

void HandleUnlink(int chl, struct fsm_req_unlink* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
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

	bool result = inode->Unlink(path, false);
	free(path);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondSuccess(chl);
}

void HandleRemoveDir(int chl, struct fsm_req_rmdir* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
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

	bool result = inode->RemoveDirectory(path);
	free(path);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondSuccess(chl);
}

void HandleLink(int chl, struct fsm_req_link* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
	if ( !inode ) { RespondError(chl, errno); return; }
	Inode* dest = SafeGetInode(fs, msg->linkino);
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

	bool result = inode->Link(path, dest, false);

	free(path);
	dest->Unref();
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondSuccess(chl);
}

void HandleSymlink(int chl, struct fsm_req_symlink* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->dirino);
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
		free(dest);
		RespondError(chl, errno);
		inode->Unref();
		return;
	}
	memcpy(path, path_raw, msg->namelen);
	path[msg->namelen] = '\0';

	bool result = inode->Symlink(path, dest);

	free(path);
	free(dest);
	inode->Unref();

	if ( !result ) { RespondError(chl, errno); return; }

	RespondSuccess(chl);
}

void HandleReadlink(int chl, struct fsm_req_readlink* msg, Filesystem* fs)
{
	Inode* inode = SafeGetInode(fs, msg->ino);
	if ( !inode ) { RespondError(chl, errno); return; }
	if ( !EXT2_S_ISLNK(inode->Mode()) ) { inode->Unref(); RespondError(chl, EINVAL); return; }
	size_t count = inode->Size();
	uint8_t* buf = (uint8_t*) malloc(count);
	if ( !buf ) { inode->Unref(); RespondError(chl, errno); return; }
	ssize_t amount = inode->ReadAt(buf, count, 0);
	inode->Unref();
	if ( amount < 0 )  { RespondError(chl, errno); return; }
	RespondReadlink(chl, buf, amount);
	free(buf);
}

void HandleRename(int chl, struct fsm_req_rename* msg, Filesystem* fs)
{
	char* pathraw = (char*) &(msg[1]);
	char* path = (char*) malloc(msg->oldnamelen+1 + msg->newnamelen+1);
	if ( !path ) { RespondError(chl, errno); return; }
	memcpy(path, pathraw, msg->oldnamelen);
	path[msg->oldnamelen] = '\0';
	memcpy(path + msg->oldnamelen + 1, pathraw + msg->oldnamelen, msg->newnamelen);
	path[msg->oldnamelen + 1 + msg->newnamelen] = '\0';

	const char* oldname = path;
	const char* newname = path + msg->oldnamelen + 1;

	Inode* olddir = SafeGetInode(fs, msg->olddirino);
	if ( !olddir ) { free(path); RespondError(chl, errno); return; }
	Inode* newdir = SafeGetInode(fs, msg->newdirino);
	if ( !newdir ) { olddir->Unref(); free(path); RespondError(chl, errno); return; }

	bool result = newdir->Rename(olddir, oldname, newname);

	newdir->Unref();
	olddir->Unref();
	free(path);

	if ( !result ) { RespondError(chl, errno); return; }

	RespondSuccess(chl);
}

void HandleStatVFS(int chl, struct fsm_req_statvfs* msg, Filesystem* fs)
{
	(void) msg;
	struct statvfs stvfs;
	stvfs.f_bsize = fs->block_size;
	stvfs.f_frsize = fs->block_size;
	stvfs.f_blocks = fs->num_blocks;
	stvfs.f_bfree = fs->sb->s_free_blocks_count;
	stvfs.f_bavail = fs->sb->s_free_blocks_count;
	stvfs.f_files = fs->num_inodes;
	stvfs.f_ffree = fs->sb->s_free_inodes_count;
	stvfs.f_favail = fs->sb->s_free_inodes_count;
	stvfs.f_ffree = fs->sb->s_free_inodes_count;
	stvfs.f_fsid = 0;
	stvfs.f_flag = 0;
	if ( !fs->device->write )
		stvfs.f_flag |= ST_RDONLY;
	stvfs.f_namemax = 255;
	RespondStatVFS(chl, &stvfs);
}

void HandleTCGetBlob(int chl, struct fsm_req_tcgetblob* msg, Filesystem* fs)
{
	char* nameraw = (char*) &(msg[1]);
	char* name = (char*) malloc(msg->namelen + 1);
	if ( !name )
		return (void) RespondError(chl, errno);
	memcpy(name, nameraw, msg->namelen);
	name[msg->namelen] = '\0';

	static const char index[] = "device-path\0filesystem-type\0filesystem-uuid\0mount-path\0";
	if ( !strcmp(name, "") )
		RespondTCGetBlob(chl, index, sizeof(index) - 1);
	else if ( !strcmp(name, "device-path") )
		RespondTCGetBlob(chl, fs->device->path, strlen(fs->device->path));
	else if ( !strcmp(name, "filesystem-type") )
		RespondTCGetBlob(chl, "ext2", strlen("ext2"));
	else if ( !strcmp(name, "filesystem-uuid") )
		RespondTCGetBlob(chl, fs->sb->s_uuid, sizeof(fs->sb->s_uuid));
	else if ( !strcmp(name, "mount-path") )
		RespondTCGetBlob(chl, fs->mount_path, strlen(fs->mount_path));
	else
		RespondError(chl, ENOENT);

	free(name);
}

void HandleIncomingMessage(int chl, struct fsm_msg_header* hdr, Filesystem* fs)
{
	request_uid = hdr->uid;
	request_gid = hdr->gid;
	if ( (uint16_t) request_uid != request_uid ||
	     (uint16_t) request_gid != request_gid )
	{
		fprintf(stderr, "extfs: id exceeded 16-bit: uid=%ju gid=%ju\n",
		        (uintmax_t) request_uid, (uintmax_t) request_gid);
		RespondError(chl, EOVERFLOW);
		return;
	}
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
	handlers[FSM_REQ_REFER] = (handler_t) HandleRefer;
	handlers[FSM_REQ_UNREF] = (handler_t) HandleUnref;
	handlers[FSM_REQ_STATVFS] = (handler_t) HandleStatVFS;
	handlers[FSM_REQ_TCGETBLOB] = (handler_t) HandleTCGetBlob;
	if ( FSM_MSG_NUM <= hdr->msgtype || !handlers[hdr->msgtype] )
	{
		fprintf(stderr, "extfs: message type %zu not supported\n", hdr->msgtype);
		RespondError(chl, ENOTSUP);
		return;
	}
	uint8_t body_buffer[65536];
	uint8_t* body = body_buffer;
	if ( sizeof(body_buffer) < hdr->msgsize )
	{
		body = (uint8_t*) mmap(NULL, hdr->msgsize, PROT_READ | PROT_WRITE,
		                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if ( (void*) body == MAP_FAILED )
		{
			RespondError(chl, errno);
			return;
		}
	}
	if ( readall(chl, body, hdr->msgsize) == hdr->msgsize )
		handlers[hdr->msgtype](chl, body, fs);
	else
		RespondError(chl, errno);
	if ( sizeof(body_buffer) < hdr->msgsize )
		munmap(body, hdr->msgsize);
}
static volatile bool should_terminate = false;

void TerminationHandler(int)
{
	should_terminate = true;
}

int fsmarshall_main(const char* argv0,
                    const char* mount_path,
                    bool foreground,
                    Filesystem* fs,
                    Device* dev)
{
	(void) argv0;

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

	dev->SpawnSyncThread();

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
			//error(0, errno, "incomplete header: got %zi of %zu bytes", amount, sizeof(hdr));
			errno = 0;
			continue;
		}
		HandleIncomingMessage(channel, &hdr, fs);
		close(channel);

		if ( dev->write && !dev->has_sync_thread )
		{
			struct timespec now;
			clock_gettime(CLOCK_MONOTONIC, &now);

			if ( 5 <= timespec_sub(now, last_sync_at).tv_sec )
			{
				fs->Sync();
				last_sync_at = now;
			}
		}
	}

	// Garbage collect all open inode references.
	while ( fs->mru_inode )
	{
		Inode* inode = fs->mru_inode;
		if ( inode->remote_reference_count )
			inode->RemoteUnref();
		else if ( inode->reference_count )
			inode->Unref();
	}

	// Sync the filesystem before shutting down.
	if ( dev->write )
		fs->Sync();

	close(serverfd);

	delete fs;
	delete dev;

	return 0;
}

#endif
