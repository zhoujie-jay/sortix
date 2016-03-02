/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * sortix/initrd.h
 * The Sortix init ramdisk filesystem format.
 */

#ifndef INCLUDE_SORTIX_INITRD_H
#define INCLUDE_SORTIX_INITRD_H

#include <stdint.h>

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INITRD_ALGO_CRC32 0

#define INITRD_S_IXOTH 01
#define INITRD_S_IWOTH 02
#define INITRD_S_IROTH 03
#define INITRD_S_IRWXO 07
#define INITRD_S_IXGRP 010
#define INITRD_S_IWGRP 020
#define INITRD_S_IRGRP 040
#define INITRD_S_IRWXG 070
#define INITRD_S_IXUSR 0100
#define INITRD_S_IWUSR 0200
#define INITRD_S_IRUSR 0400
#define INITRD_S_IRWXU 0700
#define INITRD_S_IFMT 0xF000
#define INITRD_S_IFSOCK 0xC000
#define INITRD_S_IFLNK 0xA000
#define INITRD_S_IFREG 0x8000
#define INITRD_S_IFBLK 0x6000
#define INITRD_S_IFDIR 0x4000
#define INITRD_S_IFCHR 0x2000
#define INITRD_S_IFIFO 0x1000
#define INITRD_S_ISUID 0x0800
#define INITRD_S_ISGID 0x0400
#define INITRD_S_ISVTX 0x0200
#define INITRD_S_ISSOCK(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFSOCK)
#define INITRD_S_ISLNK(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFLNK)
#define INITRD_S_ISREG(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFREG)
#define INITRD_S_ISBLK(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFBLK)
#define INITRD_S_ISDIR(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFDIR)
#define INITRD_S_ISCHR(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFCHR)
#define INITRD_S_ISFIFO(mode) ((mode & INITRD_S_IFMT) == INITRD_S_IFIFO)

typedef struct initrd_superblock
{
	char magic[16]; // "sortix-initrd-2"
	uint32_t fssize;
	uint32_t revision;
	uint32_t inodesize;
	uint32_t inodecount;
	uint32_t inodeoffset;
	uint32_t root;
	uint32_t sumalgorithm;
	uint32_t sumsize;
} initrd_superblock_t;

typedef struct initrd_inode
{
	uint32_t mode;
	uint32_t uid;
	uint32_t gid;
	uint32_t nlink;
	uint64_t ctime;
	uint64_t mtime;
	uint32_t dataoffset;
	uint32_t size;
} initrd_inode_t;

typedef struct initrd_dirent
{
	uint32_t inode;
	uint16_t reclen;
	uint16_t namelen;
	char name[0];
} initrd_dirent_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
