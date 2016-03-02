/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/fcntl.h
 * Declares various constants related to opening files.
 */

#ifndef INCLUDE_SORTIX_FCNTL_H
#define INCLUDE_SORTIX_FCNTL_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Remember to update the flag classifications at the top of descriptor.cpp if
   you add new flags here. */
#define O_READ (1<<0)
#define O_WRITE (1<<1)
#define O_EXEC (1<<2)
#define O_APPEND (1<<3)
#define O_CLOEXEC (1<<4)
#define O_CREATE (1<<5)
#define O_DIRECTORY (1<<6)
#define O_EXCL (1<<7)
#define O_TRUNC (1<<8)
#define O_CLOFORK (1<<9)
#define O_SEARCH (1<<10)
#define O_NONBLOCK (1<<11)
#define O_NOFOLLOW (1<<12)
#define O_SYMLINK_NOFOLLOW (1<<13)
#define O_NOCTTY (1<<14)
#define O_TTY_INIT (1<<15)

#define O_ACCMODE (O_READ | O_WRITE | O_EXEC | O_SEARCH)

#define FD_CLOEXEC (1<<0)
#define FD_CLOFORK (1<<1)

#define __FD_ALLOWED_FLAGS (FD_CLOEXEC | FD_CLOFORK)

/* Encode type information about arguments into fcntl commands. Unfortunately
   the fcntl function declaration doesn't include type information, which means
   that the fcntl implementation either needs a list of command type information
   to use va_list correctly - or we can simply embed the information into the
   commands. */
#define F_TYPE_EXP 3 /* 2^3 kinds of argument types supported.*/
#define F_TYPE_VOID 0
#define F_TYPE_INT 1
#define F_TYPE_LONG 2
#define F_TYPE_PTR 3
/* 5-7 is unused in case of future expansion. */
#define F_ENCODE_CMD(cmd_val, arg_type) ((cmd_val) << F_TYPE_EXP | (arg_type))
#define F_DECODE_CMD_RAW(cmd) (cmd >> F_TYPE_EXP)
#define F_DECODE_CMD_TYPE(cmd) ((cmd) & ((1 << F_TYPE_EXP)-1))

/* Encode small parameters into fcntl commands. This allows adding some flags
   and other modifiers to fcntl commands without declaring a heap of new fcntl
   commands variants. */
#define F_ENCODE(cmd, small_param) (((cmd) & 0xFFFF) | ((small_param) << 16))
#define F_DECODE_CMD(val) (val & 0xFFFF)
#define F_DECODE_FLAGS(val) ((val & ~0xFFFF) >> 16)

/* Set file descriptor status. */
#define F_SETFD_NUM 0
#define F_SETFD F_ENCODE_CMD(F_SETFD_NUM, F_TYPE_INT)

/* Get file descriptor status. */
#define F_GETFD_NUM 1
#define F_GETFD F_ENCODE_CMD(F_GETFD_NUM, F_TYPE_VOID)

/* Set descriptor table entry flags. */
#define F_SETFL_NUM 3
#define F_SETFL F_ENCODE_CMD(F_SETFL_NUM, F_TYPE_INT)

/* Get descriptor table entry flags. */
#define F_GETFL_NUM 3
#define F_GETFL F_ENCODE_CMD(F_GETFL_NUM, F_TYPE_VOID)

/* Duplicates the file descriptor with at least the given index. */
#define F_DUPFD_NUM 4
#define F_DUPFD F_ENCODE_CMD(F_ENCODE(F_DUPFD_NUM, 0), F_TYPE_INT)
#define F_DUPFD_CLOEXEC F_ENCODE_CMD(F_ENCODE(F_DUPFD_NUM, FD_CLOEXEC), F_TYPE_INT)
#define F_DUPFD_CLOFORK F_ENCODE_CMD(F_ENCODE(F_DUPFD_NUM, FD_CLOFORK), F_TYPE_INT)

/* Find the previous file descriptor. */
#define F_PREVFD_NUM 5
#define F_PREVFD F_ENCODE_CMD(F_PREVFD_NUM, F_TYPE_VOID)

/* Find the next file descriptor. */
#define F_NEXTFD_NUM 6
#define F_NEXTFD F_ENCODE_CMD(F_NEXTFD_NUM, F_TYPE_VOID)

#define AT_FDCWD (-100)
#define AT_REMOVEDIR (1<<0)
#define AT_EACCESS (1<<1)
#define AT_SYMLINK_NOFOLLOW (1<<2)
#define AT_REMOVEFILE (1<<3)
#define AT_SYMLINK_FOLLOW (1<<4)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
