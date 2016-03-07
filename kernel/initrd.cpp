/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * initrd.cpp
 * Extracts initrds into the initial memory filesystem.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <timespec.h>

#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/initrd.h>
#include <sortix/mman.h>
#include <sortix/stat.h>
#include <sortix/tar.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/vnode.h>

#include "initrd.h"
#include "multiboot.h"

namespace Sortix {

// TODO: The initrd is not being properly verified.
// TODO: The initrd is not handled in an endian-neutral manner.

struct initrd_context
{
	uint8_t* initrd;
	size_t initrd_size;
	addr_t initrd_unmap_start;
	addr_t initrd_unmap_end;
	struct initrd_superblock* sb;
	Ref<Descriptor> links;
	ioctx_t ioctx;
};

// TODO: GRUB is currently buggy and doesn't ensure that other things are placed
//       at the end of a module, i.e. that the module doesn't own all the bugs
//       that it spans. It's thus risky to actually recycle the last page if the
//       module doesn't use all of it. Remove this compatibility when this has
//       been fixed in GRUB and a few years have passed such that most GRUB
//       systems have this fixed.
static void UnmapInitrdPage(struct initrd_context* ctx, addr_t vaddr)
{
	if ( !Memory::LookUp(vaddr, NULL, NULL) )
		return;
	addr_t addr = Memory::Unmap(vaddr);
	if ( !(ctx->initrd_unmap_start <= addr && addr < ctx->initrd_unmap_end) )
		return;
	Page::Put(addr, PAGE_USAGE_WASNT_ALLOCATED);
}

static mode_t initrd_mode_to_host_mode(uint32_t mode)
{
	mode_t result = mode & 0777;
	if ( INITRD_S_ISVTX & mode ) result |= S_ISVTX;
	if ( INITRD_S_ISSOCK(mode) ) result |= S_IFSOCK;
	if ( INITRD_S_ISLNK(mode) ) result |= S_IFLNK;
	if ( INITRD_S_ISREG(mode) ) result |= S_IFREG;
	if ( INITRD_S_ISBLK(mode) ) result |= S_IFBLK;
	if ( INITRD_S_ISDIR(mode) ) result |= S_IFDIR;
	if ( INITRD_S_ISCHR(mode) ) result |= S_IFCHR;
	if ( INITRD_S_ISFIFO(mode) ) result |= S_IFIFO;
	return result;
}

static struct initrd_inode* initrd_get_inode(struct initrd_context* ctx,
                                             uint32_t inode)
{
	if ( ctx->sb->inodecount <= inode )
		return errno = EINVAL, (struct initrd_inode*) NULL;
	uint32_t pos = ctx->sb->inodeoffset + ctx->sb->inodesize * inode;
	return (struct initrd_inode*) (ctx->initrd + pos);
}

static uint8_t* initrd_inode_get_data(struct initrd_context* ctx,
                                      struct initrd_inode* inode,
                                      size_t* size)
{
	return *size = inode->size, ctx->initrd + inode->dataoffset;
}

static uint32_t initrd_directory_open(struct initrd_context* ctx,
                                      struct initrd_inode* inode,
                                      const char* name)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, 0;
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		struct initrd_dirent* dirent =
			(struct initrd_dirent*) (ctx->initrd + pos);
		if ( dirent->namelen && !strcmp(dirent->name, name) )
			return dirent->inode;
		offset += dirent->reclen;
	}
	return errno = ENOENT, 0;
}

static const char* initrd_directory_get_filename(struct initrd_context* ctx,
                                                 struct initrd_inode* inode,
                                                 size_t index)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, (const char*) NULL;
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		struct initrd_dirent* dirent =
			(struct initrd_dirent*) (ctx->initrd + pos);
		if ( index-- == 0 )
			return dirent->name;
		offset += dirent->reclen;
	}
	return errno = EINVAL, (const char*) NULL;
}

static size_t initrd_directory_get_num_files(struct initrd_context* ctx,
                                             struct initrd_inode* inode)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, 0;
	uint32_t offset = 0;
	size_t numentries = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		const struct initrd_dirent* dirent =
			(const struct initrd_dirent*) (ctx->initrd + pos);
		numentries++;
		offset += dirent->reclen;
	}
	return numentries;
}

static void ExtractNode(struct initrd_context* ctx,
                        struct initrd_inode* inode,
                        Ref<Descriptor> node);

static void ExtractFile(struct initrd_context* ctx,
                        struct initrd_inode* inode,
                        Ref<Descriptor> file)
{
	size_t filesize;
	uint8_t* data = initrd_inode_get_data(ctx, inode, &filesize);
	if ( file->truncate(&ctx->ioctx, filesize) != 0 )
		PanicF("initrd: truncate: %m");
	size_t sofar = 0;
	while ( sofar < filesize )
	{
		size_t left = filesize - sofar;
		size_t chunk = 1024 * 1024;
		size_t count = left < chunk ? left : chunk;
		ssize_t numbytes = file->write(&ctx->ioctx, data + sofar, count);
		if ( numbytes <= 0 )
			PanicF("initrd: write: %m");
		sofar += numbytes;
	}
}

static void ExtractDir(struct initrd_context* ctx,
                       struct initrd_inode* inode,
                       Ref<Descriptor> dir)
{
	size_t numfiles = initrd_directory_get_num_files(ctx, inode);
	for ( size_t i = 0; i < numfiles; i++ )
	{
		const char* name = initrd_directory_get_filename(ctx, inode, i);
		if ( !name )
			PanicF("initrd_directory_get_filename: %m");
		if ( IsDotOrDotDot(name) )
			continue;
		uint32_t childino = initrd_directory_open(ctx, inode, name);
		if ( !childino )
			PanicF("initrd_directory_open: %s: %m", name);
		struct initrd_inode* child =
			(struct initrd_inode*) initrd_get_inode(ctx, childino);
		mode_t mode = initrd_mode_to_host_mode(child->mode);
		if ( INITRD_S_ISDIR(child->mode) )
		{
			if ( dir->mkdir(&ctx->ioctx, name, mode) && errno != EEXIST )
				PanicF("initrd: mkdir: %s: %m", name);
			Ref<Descriptor> desc = dir->open(&ctx->ioctx, name,
			                                 O_SEARCH | O_DIRECTORY, 0);
			if ( !desc )
				PanicF("initrd: %s: %m", name);
			ExtractNode(ctx, child, desc);
		}
		if ( INITRD_S_ISREG(child->mode) )
		{
			assert(child->nlink != 0);
			char link_path[sizeof(childino) * 3];
			snprintf(link_path, sizeof(link_path), "%ju", (uintmax_t) childino);
			Ref<Descriptor> existing(ctx->links->open(&ctx->ioctx, link_path,
			                         O_READ, 0));
			if ( !existing || dir->link(&ctx->ioctx, name, existing) != 0 )
			{
				Ref<Descriptor> desc(dir->open(&ctx->ioctx, name,
				                               O_WRITE | O_CREATE, mode));
				if ( !desc )
					PanicF("initrd: %s: %m", name);
				ExtractNode(ctx, child, desc);
				if ( 2 <= child->nlink )
					ctx->links->link(&ctx->ioctx, link_path, desc);
			}
			if ( --child->nlink == 0 && INITRD_S_ISREG(child->mode) )
			{
				size_t filesize;
				const uint8_t* data =
					initrd_inode_get_data(ctx, child, &filesize);
				uintptr_t from = (uintptr_t) data;
				uintptr_t size = filesize;
				uintptr_t from_aligned = Page::AlignUp(from);
				uintptr_t from_distance = from_aligned - from;
				if ( from_distance <= size )
				{
					uintptr_t size_aligned =
						Page::AlignDown(size - from_distance);
					for ( size_t i = 0; i < size_aligned; i += Page::Size() )
						UnmapInitrdPage(ctx, from_aligned + i);
					Memory::Flush();
				}
			}
		}
		if ( INITRD_S_ISLNK(child->mode) )
		{
			size_t filesize;
			uint8_t* data = initrd_inode_get_data(ctx, child, &filesize);
			char* oldname = new char[filesize + 1];
			if ( !oldname )
				PanicF("initrd: malloc: %m");
			memcpy(oldname, data, filesize);
			oldname[filesize] = '\0';
			int ret = dir->symlink(&ctx->ioctx, oldname, name);
			delete[] oldname;
			if ( ret < 0 )
				PanicF("initrd: symlink: %s", name);
			Ref<Descriptor> desc = dir->open(&ctx->ioctx, name,
			                                 O_READ | O_SYMLINK_NOFOLLOW, 0);
			if ( desc )
				ExtractNode(ctx, child, desc);
		}
	}
}

static void ExtractNode(struct initrd_context* ctx,
                        struct initrd_inode* inode,
                        Ref<Descriptor> node)
{
	if ( node->chmod(&ctx->ioctx, initrd_mode_to_host_mode(inode->mode)) < 0 )
		PanicF("initrd: chmod: %m");
	if ( node->chown(&ctx->ioctx, inode->uid, inode->gid) < 0 )
		PanicF("initrd: chown: %m");
	if ( INITRD_S_ISDIR(inode->mode) )
		ExtractDir(ctx, inode, node);
	if ( INITRD_S_ISREG(inode->mode) )
		ExtractFile(ctx, inode, node);
	struct timespec times[2];
	times[0] = timespec_make((time_t) inode->mtime, 0);
	times[1] = timespec_make((time_t) inode->mtime, 0);
	if ( node->utimens(&ctx->ioctx, times) < 0 )
		PanicF("initrd: utimens: %m");
}

static void ExtractInitrd(Ref<Descriptor> desc, struct initrd_context* ctx)
{
	ctx->sb = (struct initrd_superblock*) ctx->initrd;

	if ( ctx->initrd_size < ctx->sb->fssize )
		Panic("Initrd header does not match its size");

	if ( desc->mkdir(&ctx->ioctx, ".initrd-links", 0777) != 0 )
		PanicF("initrd: .initrd-links: %m");

	if ( !(ctx->links = desc->open(&ctx->ioctx, ".initrd-links",
	                               O_READ | O_DIRECTORY, 0)) )
		PanicF("initrd: .initrd-links: %m");

	ExtractNode(ctx, initrd_get_inode(ctx, ctx->sb->root), desc);

	union
	{
		struct dirent dirent;
		uint8_t dirent_data[sizeof(struct dirent) + sizeof(uintmax_t) * 3];
	};

	while ( 0 < ctx->links->readdirents(&ctx->ioctx, &dirent, sizeof(dirent_data)) &&
	        ((const char*) dirent.d_name)[0] )
	{
		if ( ((const char*) dirent.d_name)[0] == '.' )
			continue;
		ctx->links->unlinkat(&ctx->ioctx, dirent.d_name, AT_REMOVEFILE);
		ctx->links->lseek(&ctx->ioctx, 0, SEEK_SET);
	}

	ctx->links.Reset();

	desc->unlinkat(&ctx->ioctx, ".initrd-links", AT_REMOVEDIR);
}

struct TAR
{
	unsigned char* tar_file;
	size_t tar_file_size;
	size_t next_offset;
	size_t offset;
	size_t data_offset;
	char* name;
	char* linkname;
	unsigned char* data;
	size_t size;
	mode_t mode;
	char typeflag;
};

static void OpenTar(TAR* TAR, unsigned char* tar_file, size_t tar_file_size)
{
	memset(TAR, 0, sizeof(*TAR));
	TAR->tar_file = tar_file;
	TAR->tar_file_size = tar_file_size;
}

static void CloseTar(TAR* TAR)
{
	free(TAR->name);
	free(TAR->linkname);
	memset(TAR, 0, sizeof(*TAR));
}

static bool ReadTar(TAR* TAR)
{
	free(TAR->name);
	free(TAR->linkname);
	TAR->name = NULL;
	TAR->linkname = NULL;
	while ( true )
	{
		if ( TAR->tar_file_size - TAR->next_offset < sizeof(struct TAR) )
			return false;
		TAR->offset = TAR->next_offset;
		struct tar* tar = (struct tar*) (TAR->tar_file + TAR->offset);
		if ( tar->size[sizeof(tar->size) - 1] != '\0' )
			return false;
		size_t size = strtoul(tar->size, NULL, 8);
		size_t dist = sizeof(struct tar) + -(-size & ~((size_t) 512 - 1));
		if ( TAR->tar_file_size - TAR->offset < dist )
			return false;
		TAR->next_offset = TAR->offset + dist;
		TAR->data_offset = TAR->offset + 512;
		TAR->data = TAR->tar_file + TAR->data_offset;
		TAR->size = size;
		if ( tar->mode[sizeof(tar->mode) - 1] != '\0' )
			return false;
		TAR->mode = strtoul(tar->mode, NULL, 8) & 07777;
		TAR->typeflag = tar->typeflag;
		// TODO: Things like modified time and other meta data!
		if ( tar->typeflag == 'L' )
		{
			free(TAR->name);
			if ( !(TAR->name = (char*) malloc(size + 1)) )
				Panic("initrd tar malloc failure");
			memcpy(TAR->name, TAR->data, size);
			TAR->name[size] = '\0';
			continue;
		}
		else if ( tar->typeflag == 'g' )
		{
			// TODO: Implement pax extensions.
			continue;
		}
		else if ( tar->typeflag == 'x' )
		{
			// TODO: Implement pax extensions.
			continue;
		}
		if ( !tar->name[0] )
			continue;
		if ( !TAR->name )
		{
			if ( tar->prefix[0] )
			{
				size_t prefix_len = strnlen(tar->prefix, sizeof(tar->prefix));
				size_t name_len = strnlen(tar->name, sizeof(tar->name));
				size_t name_size = prefix_len + 1 + name_len + 1;
				if ( !(TAR->name = (char*) malloc(name_size)) )
					Panic("initrd tar malloc failure");
				memcpy(TAR->name, tar->prefix, prefix_len);
				TAR->name[prefix_len] = '/';
				memcpy(TAR->name + prefix_len + 1, tar->name, name_len);
				TAR->name[prefix_len + 1 + name_len] = '\0';
			}
			else
			{
				TAR->name = (char*) strndup(tar->name, sizeof(tar->name));
				if ( !TAR->name )
					Panic("initrd tar malloc failure");
			}
		}
		if ( !TAR->linkname )
		{
			TAR->linkname = (char*) strndup(tar->linkname, sizeof(tar->linkname));
			if ( !TAR->linkname )
				Panic("initrd tar malloc failure");
		}
		return true;
	}
}

static bool SearchTar(struct initrd_context* ctx, TAR* TAR, const char* path)
{
	OpenTar(TAR, ctx->initrd, ctx->initrd_size);
	while ( ReadTar(TAR) )
	{
		if ( !strcmp(TAR->name, path) )
			return true;
	}
	CloseTar(TAR);
	return false;
}

static void ExtractTarObject(Ref<Descriptor> desc,
                             struct initrd_context* ctx,
                             TAR* TAR)
{
	if ( TAR->typeflag == '0' )
	{
		int oflags = O_WRITE | O_CREATE | O_TRUNC;
		Ref<Descriptor> file(desc->open(&ctx->ioctx, TAR->name, oflags, TAR->mode));
		if ( !file )
			PanicF("%s: %m", TAR->name);
		if ( file->truncate(&ctx->ioctx, TAR->size) != 0 )
			PanicF("truncate: %s: %m", TAR->name);
		size_t sofar = 0;
		while ( sofar < TAR->size )
		{
			size_t left = TAR->size - sofar;
			size_t chunk = 1024 * 1024;
			size_t count = left < chunk ? left : chunk;
			ssize_t numbytes = file->write(&ctx->ioctx, TAR->data + sofar, count);
			if ( numbytes <= 0 )
				PanicF("write: %s: %m", TAR->name);
			sofar += numbytes;
		}
	}
	else if ( TAR->typeflag == '1' )
	{
		Ref<Descriptor> dest(desc->open(&ctx->ioctx, TAR->linkname, O_READ, 0));
		if ( !dest )
			PanicF("%s: %m", TAR->linkname);
		if ( desc->link(&ctx->ioctx, TAR->name, dest) != 0 )
			PanicF("link: %s -> %s: %m", TAR->linkname, TAR->name);
	}
	else if ( TAR->typeflag == '2' )
	{
		if ( desc->symlink(&ctx->ioctx, TAR->linkname, TAR->name) != 0 )
			PanicF("symlink: %s: %m", TAR->name);
	}
	else if ( TAR->typeflag == '5' )
	{
		if ( desc->mkdir(&ctx->ioctx, TAR->name, TAR->mode) && errno != EEXIST )
			PanicF("mkdir: %s: %m", TAR->name);
	}
	else
	{
		Log::PrintF("kernel: initrd: %s: Unsupported tar filetype '%c'\n",
		            TAR->name, TAR->typeflag);
	}
}

static void ExtractTar(Ref<Descriptor> desc, struct initrd_context* ctx)
{
	TAR TAR;
	OpenTar(&TAR, ctx->initrd, ctx->initrd_size);
	while ( ReadTar(&TAR) )
		ExtractTarObject(desc, ctx, &TAR);
	CloseTar(&TAR);
}

static bool TarIsTix(struct initrd_context* ctx)
{
	TAR TAR;
	bool result = SearchTar(ctx, &TAR, "tix/tixinfo");
	CloseTar(&TAR);
	return result;
}

static char* tixinfo_lookup(const char* info,
                            size_t info_size,
                            const char* what)
{
	size_t what_length = strlen(what);
	while ( info_size )
	{
		size_t line_length = 0;
		while ( line_length < info_size && info[line_length] != '\n' )
			line_length++;
		if ( what_length <= line_length &&
		     !strncmp(info, what, what_length) &&
		     info[what_length] == '=' )
		{
			char* result = strndup(info + what_length + 1,
			                       line_length - (what_length + 1));
			if ( !result )
				Panic("initrd tar malloc failure");
			return result;
		}
		info += line_length;
		info_size -= line_length;
		if ( info_size && info[0] == '\n' )
		{
			info++;
			info_size--;
		}
	}
	return NULL;
}

static void DescriptorWriteLine(Ref<Descriptor> desc,
                                ioctx_t* ioctx,
                                const char* str)
{
	size_t len = strlen(str);
	while ( str[0] )
	{
		ssize_t done = desc->write(ioctx, (unsigned char*) str, len);
		if ( done <= 0 )
			PanicF("initrd tix metadata write: %m");
		str += done;
		len -= done;
	}
	if ( desc->write(ioctx, (unsigned char*) "\n", 1) != 1 )
		PanicF("initrd tix metadata write: %m");
}

static int manifest_sort(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return strcmp(a, b);
}

static void ExtractTix(Ref<Descriptor> desc, struct initrd_context* ctx)
{
	TAR TAR;
	if ( !SearchTar(ctx, &TAR, "tix/tixinfo") )
		Panic("initrd was not tix");
	char* pkg_name =
		tixinfo_lookup((const char*) TAR.data, TAR.size, "pkg.name");
	if ( !pkg_name )
		Panic("initrd tixinfo lacked pkg.name");
	if ( desc->mkdir(&ctx->ioctx, "/tix", 0755) < 0 && errno != EEXIST )
		PanicF("/tix: %m");
	if ( desc->mkdir(&ctx->ioctx, "/tix/tixinfo", 0755) < 0 && errno != EEXIST )
		PanicF("/tix/tixinfo: %m");
	if ( desc->mkdir(&ctx->ioctx, "/tix/manifest", 0755) < 0 && errno != EEXIST )
		PanicF("/tix/manifest: %m");
	char* tixinfo_path;
	if ( asprintf(&tixinfo_path, "/tix/tixinfo/%s", pkg_name) < 0 )
		Panic("initrd tar malloc failure");
	char* TAR_oldname = TAR.name;
	TAR.name = tixinfo_path;
	ExtractTarObject(desc, ctx, &TAR);
	TAR.name = TAR_oldname;
	free(tixinfo_path);
	CloseTar(&TAR);
	Ref<Descriptor> installed_list =
		desc->open(&ctx->ioctx, "/tix/installed.list",
		           O_CREATE | O_WRITE | O_APPEND, 0644);
	if ( !installed_list )
		PanicF("/tix/installed.list: %m");
	DescriptorWriteLine(installed_list, &ctx->ioctx, pkg_name);
	installed_list.Reset();
	size_t manifest_list_size = 0;
	OpenTar(&TAR, ctx->initrd, ctx->initrd_size);
	while ( ReadTar(&TAR) )
	{
		if ( !strncmp(TAR.name, "data", 4) && TAR.name[4] == '/' )
			manifest_list_size++;
	}
	CloseTar(&TAR);
	char** manifest_list = new char*[manifest_list_size];
	if ( !manifest_list )
		Panic("initrd tar malloc failure");
	OpenTar(&TAR, ctx->initrd, ctx->initrd_size);
	size_t manifest_list_count = 0;
	while ( ReadTar(&TAR) )
	{
		if ( strncmp(TAR.name, "data", 4) != 0 || TAR.name[4] != '/' )
			continue;
		if ( !(manifest_list[manifest_list_count++] = strdup(TAR.name + 4)) )
			Panic("initrd tar malloc failure");
	}
	CloseTar(&TAR);
	qsort(manifest_list, manifest_list_count, sizeof(char*), manifest_sort);
	char* manifest_path;
	if ( asprintf(&manifest_path, "/tix/manifest/%s", pkg_name) < 0 )
		Panic("initrd tar malloc failure");
	Ref<Descriptor> manifest =
		desc->open(&ctx->ioctx, manifest_path, O_WRITE | O_CREATE | O_TRUNC, 0644);
	if ( !manifest )
		PanicF("%s: %m", manifest_path);
	free(manifest_path);
	for ( size_t i = 0; i < manifest_list_count; i++ )
		DescriptorWriteLine(manifest, &ctx->ioctx, manifest_list[i]);
	manifest.Reset();
	for ( size_t i = 0; i < manifest_list_count; i++ )
		free(manifest_list[i]);
	delete[] manifest_list;
	OpenTar(&TAR, ctx->initrd, ctx->initrd_size);
	const char* subdir = "data/";
	size_t subdir_length = strlen(subdir);
	while ( ReadTar(&TAR) )
	{
		bool name_data = !strncmp(TAR.name, subdir, subdir_length) &&
		                 TAR.name[subdir_length];
		bool linkname_data = !strncmp(TAR.linkname, subdir, subdir_length) &&
		                     TAR.linkname[subdir_length];
		if ( name_data )
		{
			TAR.name += subdir_length;
			if ( linkname_data )
				TAR.linkname += subdir_length;
			ExtractTarObject(desc, ctx, &TAR);
			TAR.name -= subdir_length;
			if ( linkname_data )
				TAR.linkname -= subdir_length;
		}
	}
	CloseTar(&TAR);
	free(pkg_name);
}

static int ExtractTo_mkdir(Ref<Descriptor> desc, ioctx_t* ctx,
                           const char* path, mode_t mode)
{
	int saved_errno = errno;
	if ( !desc->mkdir(ctx, path, mode) )
		return 0;
	if ( errno == ENOENT )
	{
		char* prev = strdup(path);
		if ( !prev )
			return -1;
		int status = ExtractTo_mkdir(desc, ctx, dirname(prev), mode | 0500);
		free(prev);
		if ( status < 0 )
			return -1;
		errno = saved_errno;
		if ( !desc->mkdir(ctx, path, mode) )
			return 0;
	}
	if ( errno == EEXIST )
		return errno = saved_errno, 0;
	return -1;
}

static void ExtractTo(Ref<Descriptor> desc,
                      struct initrd_context* ctx,
                      const char* path)
{
	int oflags = O_WRITE | O_CREATE | O_TRUNC;
	Ref<Descriptor> file(desc->open(&ctx->ioctx, path, oflags, 0644));
	if ( !file && errno == ENOENT )
	{
		char* prev = strdup(path);
		if ( !prev )
			PanicF("%s: strdup: %m", path);
		if ( ExtractTo_mkdir(desc, &ctx->ioctx, dirname(prev), 755) < 0 )
			PanicF("%s: mkdir -p: %s: %m", path, prev);
		free(prev);
		file = desc->open(&ctx->ioctx, path, oflags, 0644);
	}
	if ( !file )
		PanicF("%s: %m", path);
	if ( file->truncate(&ctx->ioctx, ctx->initrd_size) != 0 )
		PanicF("truncate: %s: %m", path);
	size_t sofar = 0;
	while ( sofar < ctx->initrd_size )
	{
		size_t left = ctx->initrd_size - sofar;
		size_t chunk = 1024 * 1024;
		size_t count = left < chunk ? left : chunk;
		ssize_t numbytes = file->write(&ctx->ioctx, ctx->initrd + sofar, count);
		if ( numbytes <= 0 )
			PanicF("write: %s: %m", path);
		sofar += numbytes;
	}
}

static void ExtractModule(struct multiboot_mod_list* module,
                          Ref<Descriptor> desc,
                          struct initrd_context* ctx)
{
	size_t mod_size = module->mod_end - module->mod_start;
	const char* cmdline = (const char*) (uintptr_t) module->cmdline;

	// Allocate the needed kernel virtual address space.
	addralloc_t initrd_addr_alloc;
	if ( !AllocateKernelAddress(&initrd_addr_alloc, mod_size) )
		PanicF("Failed to allocate kernel address space for the initrd");

	// Map the physical frames onto our address space.
	addr_t physfrom = module->mod_start;
	addr_t mapat = initrd_addr_alloc.from;
	for ( size_t i = 0; i < mod_size; i += Page::Size() )
	{
		if ( !Memory::Map(physfrom + i, mapat + i, PROT_KREAD | PROT_KWRITE) )
			PanicF("Unable to map the initrd into virtual memory");
	}
	Memory::Flush();

	ctx->initrd = (uint8_t*) initrd_addr_alloc.from;
	ctx->initrd_size = mod_size;
	ctx->initrd_unmap_start = module->mod_start;
	ctx->initrd_unmap_end = Page::AlignDown(module->mod_end);

	if ( !strncmp(cmdline, "--to ", strlen("--to ")) )
	{
		ExtractTo(desc, ctx, cmdline + strlen("--to "));
	}
	else if ( sizeof(struct initrd_superblock) <= ctx->initrd_size &&
	          !memcmp(ctx->initrd, "sortix-initrd-2", strlen("sortix-initrd-2")) )
	{
		ExtractInitrd(desc, ctx);
	}
	else if ( sizeof(struct tar) <= ctx->initrd_size &&
	          !memcmp(ctx->initrd + offsetof(struct tar, magic), "ustar", 5) )
	{
		if ( !strcmp(cmdline, "--tar") )
			ExtractTar(desc, ctx);
		else if ( !strcmp(cmdline, "--tix") )
			ExtractTix(desc, ctx);
		else if ( TarIsTix(ctx) )
			ExtractTix(desc, ctx);
		else
			ExtractTar(desc, ctx);
	}
	else
	{
		Panic("Unsupported initrd format, or try the --to <path> option");
	}

	// Unmap the pages and return the physical frames for reallocation.
	for ( size_t i = 0; i < mod_size; i += Page::Size() )
		UnmapInitrdPage(ctx, mapat + i);
	Memory::Flush();


	// Free the used virtual address space.
	FreeKernelAddress(&initrd_addr_alloc);
}

void ExtractModules(struct multiboot_info* bootinfo, Ref<Descriptor> root)
{
	struct multiboot_mod_list* modules =
		(struct multiboot_mod_list*) (uintptr_t) bootinfo->mods_addr;
	struct initrd_context ctx;
	memset(&ctx, 0, sizeof(ctx));
	SetupKernelIOCtx(&ctx.ioctx);
	for ( uint32_t i = 0; i < bootinfo->mods_count; i++ )
		ExtractModule(&modules[i], root, &ctx);
}

} // namespace Sortix
