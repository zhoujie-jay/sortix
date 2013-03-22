/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    fsmarshall-msg.h
    User-space filesystem API.

*******************************************************************************/

#ifndef INCLUDE_FSMARSHALL_MSG_H
#define INCLUDE_FSMARSHALL_MSG_H

#include <stddef.h>

#include <sortix/stat.h>
#include <sortix/termios.h>
#include <sortix/timespec.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct fsm_msg_header
{
	size_t msgtype;
	size_t msgsize;
};

#define FSM_RESP_ERROR 0
struct fsm_resp_error
{
	int errnum;
};

#define FSM_RESP_SUCCESS 1
struct fsm_resp_success
{
};

#define FSM_REQ_ABORT 2
struct fsm_req_abort
{
};

#define FSM_RESP_ABORT 3
struct fsm_resp_abort
{
};

#define FSM_REQ_SYNC 4
struct fsm_req_sync
{
	ino_t ino;
};

#define FSM_REQ_STAT 5
struct fsm_req_stat
{
	ino_t ino;
};

#define FSM_RESP_STAT 6
struct fsm_resp_stat
{
	struct stat st;
};

#define FSM_REQ_CHMOD 7
struct fsm_req_chmod
{
	ino_t ino;
	mode_t mode;
};

#define FSM_REQ_CHOWN 8
struct fsm_req_chown
{
	ino_t ino;
	uid_t uid;
	gid_t gid;
};

#define FSM_REQ_TRUNCATE 9
struct fsm_req_truncate
{
	ino_t ino;
	off_t size;
};

#define FSM_REQ_LSEEK 10
struct fsm_req_lseek
{
	ino_t ino;
	off_t offset;
	int whence;
};

#define FSM_RESP_LSEEK 11
struct fsm_resp_lseek
{
	off_t offset;
};

#define FSM_REQ_READ 12
struct fsm_req_read
{
	ino_t ino;
	size_t count;
};

#define FSM_REQ_PREAD 13
struct fsm_req_pread
{
	ino_t ino;
	off_t offset;
	size_t count;
};

#define FSM_RESP_READ 14
struct fsm_resp_read
{
	size_t count;
	//uint8_t data[count];
};

#define FSM_REQ_WRITE 15
struct fsm_req_write
{
	ino_t ino;
	size_t count;
	//uint8_t data[count];
};

#define FSM_REQ_PWRITE 16
struct fsm_req_pwrite
{
	ino_t ino;
	off_t offset;
	size_t count;
	//uint8_t data[count];
};

#define FSM_RESP_WRITE 17
struct fsm_resp_write
{
	size_t count;
};

#define FSM_REQ_UTIMENS 18
struct fsm_req_utimens
{
	ino_t ino;
	struct timespec times[2];
};

#define FSM_REQ_ISATTY 19
struct fsm_req_isatty
{
	ino_t ino;
};

#define FSM_REQ_READDIRENTS 20
struct fsm_req_readdirents
{
	ino_t ino;
	off_t rec_num;
};

#define FSM_RESP_READDIRENTS 21
struct fsm_resp_readdirents
{
	ino_t ino;
	unsigned char type;
	size_t namelen;
	//char name[namelen];
};

#define FSM_REQ_OPEN 22
struct fsm_req_open
{
	ino_t dirino;
	int flags;
	mode_t mode;
	size_t namelen;
	//char name[namelen];
};

#define FSM_RESP_OPEN 23
struct fsm_resp_open
{
	ino_t ino;
	mode_t type;
};

#define FSM_REQ_MKDIR 24
struct fsm_req_mkdir
{
	ino_t dirino;
	mode_t mode;
	size_t namelen;
	//char name[namelen];
};

#define FSM_RESP_MKDIR 25
struct fsm_resp_mkdir
{
	ino_t ino;
};

#define FSM_REQ_LINK 26
struct fsm_req_link
{
	ino_t dirino;
	ino_t linkino;
	size_t namelen;
	//char name[namelen];
};

#define FSM_REQ_SYMLINK 27
struct fsm_req_symlink
{
	ino_t dirino;
	size_t targetlen;
	size_t namelen;
	//char target[targetlen];
	//char name[namelen];
};

#define FSM_REQ_READLINK 28
struct fsm_req_readlink
{
	ino_t ino;
};

#define FSM_RESP_READLINK 29
struct fsm_resp_readlink
{
	size_t targetlen;
	//char target[targetlen];
};

#define FSM_REQ_TCGETWINSIZE 30
struct fsm_req_tcgetwinsize
{
	ino_t ino;
};

#define FSM_RESP_TCGETWINSIZE 31
struct fsm_resp_tcgetwinsize
{
	struct winsize size;
};

#define FSM_REQ_SETTERMMODE 32
struct fsm_req_settermmode
{
	ino_t ino;
	unsigned int termmode;
};

#define FSM_REQ_GETTERMMODE 33
struct fsm_req_gettermmode
{
	ino_t ino;
};

#define FSM_RESP_GETTERMMODE 34
struct fsm_resp_gettermmode
{
	unsigned int termmode;
};

#define FSM_REQ_UNLINK 35
struct fsm_req_unlink
{
	ino_t dirino;
	size_t namelen;
	//char name[namelen];
};

#define FSM_REQ_RMDIR 36
struct fsm_req_rmdir
{
	ino_t dirino;
	size_t namelen;
	//char name[namelen];
};

#define FSM_REQ_RENAME 37
struct fsm_req_rename
{
	ino_t olddirino;
	ino_t newdirino;
	size_t oldnamelen;
	size_t newnamelen;
	//char oldname[oldnamelen];
	//char newname[newnamelen];
};

#define FSM_MSG_NUM 38

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif
