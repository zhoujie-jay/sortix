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
 * serialize.c
 * Import and export binary data structures
 */

#include <endian.h>

#include <sortix/initrd.h>

#include "serialize.h"

void import_initrd_superblock(struct initrd_superblock* obj)
{
	obj->fssize = le32toh(obj->fssize);
	obj->revision = le32toh(obj->revision);
	obj->inodesize = le32toh(obj->inodesize);
	obj->inodecount = le32toh(obj->inodecount);
	obj->inodeoffset = le32toh(obj->inodeoffset);
	obj->root = le32toh(obj->root);
	obj->sumalgorithm = le32toh(obj->sumalgorithm);
	obj->sumsize = le32toh(obj->sumsize);
}

void export_initrd_superblock(struct initrd_superblock* obj)
{
	obj->fssize = htole32(obj->fssize);
	obj->revision = htole32(obj->revision);
	obj->inodesize = htole32(obj->inodesize);
	obj->inodecount = htole32(obj->inodecount);
	obj->inodeoffset = htole32(obj->inodeoffset);
	obj->root = htole32(obj->root);
	obj->sumalgorithm = htole32(obj->sumalgorithm);
	obj->sumsize = htole32(obj->sumsize);
}

void import_initrd_inode(struct initrd_inode* obj)
{
	obj->mode = le32toh(obj->mode);
	obj->uid = le32toh(obj->uid);
	obj->nlink = le32toh(obj->nlink);
	obj->ctime = le64toh(obj->ctime);
	obj->mtime = le64toh(obj->mtime);
	obj->dataoffset = le32toh(obj->dataoffset);
	obj->size = le32toh(obj->size);
}

void export_initrd_inode(struct initrd_inode* obj)
{
	obj->mode = htole32(obj->mode);
	obj->uid = htole32(obj->uid);
	obj->nlink = htole32(obj->nlink);
	obj->ctime = htole64(obj->ctime);
	obj->mtime = htole64(obj->mtime);
	obj->dataoffset = htole32(obj->dataoffset);
	obj->size = htole32(obj->size);
}

void import_initrd_dirent(struct initrd_dirent* obj)
{
	obj->inode = le32toh(obj->inode);
	obj->reclen = le16toh(obj->reclen);
	obj->namelen = le16toh(obj->namelen);
}

void export_initrd_dirent(struct initrd_dirent* obj)
{
	obj->inode = htole32(obj->inode);
	obj->reclen = htole16(obj->reclen);
	obj->namelen = htole16(obj->namelen);
}
