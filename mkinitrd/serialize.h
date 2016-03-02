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
 * serialize.h
 * Import and export binary data structures
 */

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <sortix/initrd.h>

void import_initrd_superblock(struct initrd_superblock* obj);
void export_initrd_superblock(struct initrd_superblock* obj);
void import_initrd_inode(struct initrd_inode* obj);
void export_initrd_inode(struct initrd_inode* obj);
void import_initrd_dirent(struct initrd_dirent* obj);
void export_initrd_dirent(struct initrd_dirent* obj);

#endif
