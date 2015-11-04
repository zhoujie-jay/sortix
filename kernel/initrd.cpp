/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    initrd.cpp
    Extracts initrds into the initial memory filesystem.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <timespec.h>

#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/initrd.h>
#include <sortix/mman.h>
#include <sortix/stat.h>

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
	struct timespec ctime = timespec_make((time_t) inode->ctime, 0);
	struct timespec mtime = timespec_make((time_t) inode->mtime, 0);
	if ( node->utimens(&ctx->ioctx, &mtime, &ctime, &mtime) < 0 )
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

static void ExtractModule(struct multiboot_mod_list* module,
                          Ref<Descriptor> desc,
                          struct initrd_context* ctx)
{
	size_t mod_size = module->mod_end - module->mod_start;

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

	if ( sizeof(struct initrd_superblock) <= ctx->initrd_size &&
	     !memcmp(ctx->initrd, "sortix-initrd-2", strlen("sortix-initrd-2")) )
	{
		ExtractInitrd(desc, ctx);
	}
	else
	{
		Panic("Unsupported initrd format");
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
