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
 * fsmarshall-msg.h
 * User-space filesystem API.
 */

#ifndef INCLUDE_FSMARSHALL_MSG_H
#define INCLUDE_FSMARSHALL_MSG_H

#include <sys/cdefs.h>

#include <stddef.h>

#include <sortix/stat.h>
#include <sortix/statvfs.h>
#include <sortix/termios.h>
#include <sortix/timespec.h>
#include <sortix/winsize.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FSM_MOUNT_CLOEXEC (1 << 0)
#define FSM_MOUNT_CLOFORK (1 << 1)
#define FSM_MOUNT_NOFOLLOW (1 << 2)
#define FSM_MOUNT_NONBLOCK (1 << 3)

struct fsm_msg_header
{
	size_t msgtype;
	size_t msgsize;
	uid_t uid;
	gid_t gid;
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
	/*uint8_t data[count];*/
};

#define FSM_REQ_WRITE 15
struct fsm_req_write
{
	ino_t ino;
	size_t count;
	/*uint8_t data[count];*/
};

#define FSM_REQ_PWRITE 16
struct fsm_req_pwrite
{
	ino_t ino;
	off_t offset;
	size_t count;
	/*uint8_t data[count];*/
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
	/*char name[namelen];*/
};

#define FSM_REQ_OPEN 22
struct fsm_req_open
{
	ino_t dirino;
	int flags;
	mode_t mode;
	size_t namelen;
	/*char name[namelen];*/
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
	/*char name[namelen];*/
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
	/*char name[namelen];*/
};

#define FSM_REQ_SYMLINK 27
struct fsm_req_symlink
{
	ino_t dirino;
	size_t targetlen;
	size_t namelen;
	/*char target[targetlen];*/
	/*char name[namelen];*/
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
	/*char target[targetlen];*/
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
	/*char name[namelen];*/
};

#define FSM_REQ_RMDIR 36
struct fsm_req_rmdir
{
	ino_t dirino;
	size_t namelen;
	/*char name[namelen];*/
};

#define FSM_REQ_RENAME 37
struct fsm_req_rename
{
	ino_t olddirino;
	ino_t newdirino;
	size_t oldnamelen;
	size_t newnamelen;
	/*char oldname[oldnamelen];*/
	/*char newname[newnamelen];*/
};

#define FSM_REQ_REFER 38
struct fsm_req_refer
{
	ino_t ino;
};

#define FSM_REQ_UNREF 39
struct fsm_req_unref
{
	ino_t ino;
};

#define FSM_REQ_TCSETPGRP 40
struct fsm_req_tcsetpgrp
{
	ino_t ino;
	pid_t pgid;
};

#define FSM_REQ_TCGETPGRP 41
struct fsm_req_tcgetpgrp
{
	ino_t ino;
};

#define FSM_RESP_TCGETPGRP 42
struct fsm_resp_tcgetpgrp
{
	pid_t pgid;
};

#define FSM_REQ_TCGETWINCURPOS 43
struct fsm_req_tcgetwincurpos
{
	ino_t ino;
};

#define FSM_REQ_STATVFS 44
struct fsm_req_statvfs
{
	ino_t ino;
};

#define FSM_RESP_STATVFS 45
struct fsm_resp_statvfs
{
	struct statvfs stvfs;
};

#define FSM_REQ_SETSOCKOPT 46
struct fsm_req_setsockopt
{
	ino_t ino;
	int level;
	int option_name;
	size_t option_size;
	/*uint8_t option[option_size];*/
};

#define FSM_REQ_GETSOCKOPT 47
struct fsm_req_getsockopt
{
	ino_t ino;
	int level;
	int option_name;
	size_t max_option_size;
};

#define FSM_RESP_GETSOCKOPT 48
struct fsm_resp_getsockopt
{
	size_t option_size;
	/*uint8_t option[option_size];*/
};

#define FSM_REQ_TCGETBLOB 49
struct fsm_req_tcgetblob
{
	ino_t ino;
	size_t namelen;
	/*char name[namelen];*/
};

#define FSM_RESP_TCGETBLOB 50
struct fsm_resp_tcgetblob
{
	size_t count;
	/*uint8_t data[count];*/
};

#define FSM_REQ_TCSETBLOB 51
struct fsm_req_tcsetblob
{
	ino_t ino;
	size_t namelen;
	size_t count;
	/*char name[namelen];*/
	/*uint8_t data[count];*/
};

#define FSM_RESP_TCSETBLOB 52
struct fsm_resp_tcsetblob
{
	size_t count;
};

#define FSM_RESP_TCGETWINCURPOS 53
struct fsm_resp_tcgetwincurpos
{
	struct wincurpos pos;
};

#define FSM_REQ_TCDRAIN 54
struct fsm_req_tcdrain
{
	ino_t ino;
};

#define FSM_REQ_TCFLOW 55
struct fsm_req_tcflow
{
	ino_t ino;
	int action;
};

#define FSM_REQ_TCFLUSH 56
struct fsm_req_tcflush
{
	ino_t ino;
	int queue_selector;
};

#define FSM_REQ_TCGETATTR 57
struct fsm_req_tcgetattr
{
	ino_t ino;
};

#define FSM_RESP_TCGETATTR 58
struct fsm_resp_tcgetattr
{
	struct termios tio;
};

#define FSM_REQ_TCGETSID 59
struct fsm_req_tcgetsid
{
	ino_t ino;
};

#define FSM_RESP_TCGETSID 60
struct fsm_resp_tcgetsid
{
	pid_t sid;
};

#define FSM_REQ_TCSENDBREAK 61
struct fsm_req_tcsendbreak
{
	ino_t ino;
	int duration;
};

#define FSM_REQ_TCSETATTR 62
struct fsm_req_tcsetattr
{
	ino_t ino;
	int actions;
	struct termios tio;
};

#define FSM_MSG_NUM 63

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
