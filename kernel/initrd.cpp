/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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
    Extracts the initrd into the initial memory filesystem.

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
#include <sortix/kernel/crc32.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/vnode.h>

#include "initrd.h"

namespace Sortix {
namespace InitRD {

// TODO: The initrd is not being properly verified.
// TODO: The initrd is not handled in an endian-neutral manner.

struct initrd_context
{
	uint8_t* initrd;
	size_t initrd_size;
	size_t amount_extracted;
	initrd_superblock_t* sb;
	Ref<Descriptor> links;
	ioctx_t ioctx;
	int last_percent;
};

void initrd_activity(const char* status, const char* format, ...)
{
	Log::PrintF("\r");
	Log::PrintF("  [%6.6s] ", status);
	va_list ap;
	va_start(ap, format);
	Log::PrintFV(format, ap);
	va_end(ap);
	Log::PrintF("...");
	size_t column, row;
	while ( Log::GetCursor(&column, &row), column != Log::Width() )
		Log::PrintF(" ");
	Log::PrintF("\e[0J");
}

void initrd_activity_done()
{
	Log::PrintF("\r\e[0J");
}

void initrd_progress(struct initrd_context* ctx)
{
	int percent = 100;
	if ( ctx->initrd_size && ctx->initrd_size != ctx->amount_extracted )
		percent = ((uint64_t) ctx->amount_extracted * 100) / ctx->initrd_size;
	if ( percent == ctx->last_percent )
		return;
	char status[6 + 1];
	snprintf(status, sizeof(status), " %3i%% ", percent);
	initrd_activity(status, "Extracting ramdisk into initial filesystem");
	ctx->last_percent = percent;
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

static initrd_inode_t* initrd_get_inode(struct initrd_context* ctx, uint32_t inode)
{
	if ( ctx->sb->inodecount <= inode )
		return errno = EINVAL, (initrd_inode_t*) NULL;
	uint32_t pos = ctx->sb->inodeoffset + ctx->sb->inodesize * inode;
	return (initrd_inode_t*) (ctx->initrd + pos);
}

static uint8_t* initrd_inode_get_data(struct initrd_context* ctx, initrd_inode_t* inode, size_t* size)
{
	return *size = inode->size, ctx->initrd + inode->dataoffset;
}

static uint32_t initrd_directory_open(struct initrd_context* ctx, initrd_inode_t* inode, const char* name)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, 0;
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		initrd_dirent* dirent = (initrd_dirent*) (ctx->initrd + pos);
		if ( dirent->namelen && !strcmp(dirent->name, name) )
			return dirent->inode;
		offset += dirent->reclen;
	}
	return errno = ENOENT, 0;
}

static const char* initrd_directory_get_filename(struct initrd_context* ctx, initrd_inode_t* inode, size_t index)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, (const char*) NULL;
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		initrd_dirent* dirent = (initrd_dirent*) (ctx->initrd + pos);
		if ( index-- == 0 )
			return dirent->name;
		offset += dirent->reclen;
	}
	return errno = EINVAL, (const char*) NULL;
}

static size_t initrd_directory_get_num_files(struct initrd_context* ctx, initrd_inode_t* inode)
{
	if ( !INITRD_S_ISDIR(inode->mode) )
		return errno = ENOTDIR, 0;
	uint32_t offset = 0;
	size_t numentries = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		const initrd_dirent* dirent = (const initrd_dirent*) (ctx->initrd + pos);
		numentries++;
		offset += dirent->reclen;
	}
	return numentries;
}

static bool ExtractNode(struct initrd_context* ctx, initrd_inode_t* inode, Ref<Descriptor> node);

static bool ExtractFile(struct initrd_context* ctx, initrd_inode_t* inode, Ref<Descriptor> file)
{
	size_t filesize;
	uint8_t* data = initrd_inode_get_data(ctx, inode, &filesize);
	if ( !data )
		return false;
	if ( file->truncate(&ctx->ioctx, filesize) != 0 )
		return false;
	size_t sofar = 0;
	while ( sofar < filesize )
	{
		size_t left = filesize - sofar;
		size_t chunk = 1024 * 1024;
		size_t count = left < chunk ? left : chunk;
		ssize_t numbytes = file->write(&ctx->ioctx, data + sofar, count);
		if ( numbytes <= 0 )
			return false;
		sofar += numbytes;
		ctx->amount_extracted += numbytes;
		initrd_progress(ctx);
	}
	return true;
}

static bool ExtractDir(struct initrd_context* ctx, initrd_inode_t* inode, Ref<Descriptor> dir)
{
	size_t numfiles = initrd_directory_get_num_files(ctx, inode);
	for ( size_t i = 0; i < numfiles; i++ )
	{
		const char* name = initrd_directory_get_filename(ctx, inode, i);
		if ( !name )
			return false;
		if ( IsDotOrDotDot(name) )
			continue;
		uint32_t childino = initrd_directory_open(ctx, inode, name);
		if ( !childino )
			return false;
		initrd_inode_t* child = (initrd_inode_t*) initrd_get_inode(ctx, childino);
		mode_t mode = initrd_mode_to_host_mode(child->mode);
		if ( INITRD_S_ISDIR(child->mode) )
		{
			if ( dir->mkdir(&ctx->ioctx, name, mode) && errno != EEXIST )
				return false;
			Ref<Descriptor> desc = dir->open(&ctx->ioctx, name, O_SEARCH | O_DIRECTORY, 0);
			if ( !desc )
				return false;
			if ( !ExtractNode(ctx, child, desc) )
				return false;
		}
		if ( INITRD_S_ISREG(child->mode) )
		{
			assert(child->nlink != 0);
			char link_path[sizeof(childino) * 3];
			snprintf(link_path, sizeof(link_path), "%ju", (uintmax_t) childino);
			Ref<Descriptor> existing(ctx->links->open(&ctx->ioctx, link_path, O_READ, 0));
			if ( !existing || dir->link(&ctx->ioctx, name, existing) != 0 )
			{
				Ref<Descriptor> desc(dir->open(&ctx->ioctx, name, O_WRITE | O_CREATE, mode));
				if ( !desc )
					return false;
				if ( !ExtractNode(ctx, child, desc) )
					return false;
				if ( 2 <= child->nlink )
					ctx->links->link(&ctx->ioctx, link_path, desc);
			}
			if ( --child->nlink == 0 && INITRD_S_ISREG(child->mode) )
			{
				size_t filesize;
				const uint8_t* data = initrd_inode_get_data(ctx, child, &filesize);
				uintptr_t from = (uintptr_t) data;
				uintptr_t size = filesize;
				uintptr_t from_aligned = Page::AlignUp(from);
				uintptr_t from_distance = from_aligned - from;
				if ( from_distance <= size )
				{
					uintptr_t size_aligned = Page::AlignDown(size - from_distance);
					for ( size_t i = 0; i < size_aligned; i += Page::Size() )
						Page::Put(Memory::Unmap(from_aligned + i), PAGE_USAGE_WASNT_ALLOCATED);
					Memory::Flush();
				}
			}
		}
		if ( INITRD_S_ISLNK(child->mode) )
		{
			size_t filesize;
			uint8_t* data = initrd_inode_get_data(ctx, child, &filesize);
			if ( !data )
				return false;
			char* oldname = new char[filesize + 1];
			memcpy(oldname, data, filesize);
			oldname[filesize] = '\0';
			int ret = dir->symlink(&ctx->ioctx, oldname, name);
			delete[] oldname;
			if ( ret < 0 )
				return false;
			Ref<Descriptor> desc = dir->open(&ctx->ioctx, name, O_READ | O_SYMLINK_NOFOLLOW, 0);
			if ( desc )
				ExtractNode(ctx, child, desc);
			ctx->amount_extracted += child->size;
			initrd_progress(ctx);
		}
	}

	ctx->amount_extracted += inode->size;
	initrd_progress(ctx);
	return true;
}

static bool ExtractNode(struct initrd_context* ctx, initrd_inode_t* inode, Ref<Descriptor> node)
{
	if ( node->chmod(&ctx->ioctx, initrd_mode_to_host_mode(inode->mode)) < 0 )
		return false;
	if ( node->chown(&ctx->ioctx, inode->uid, inode->gid) < 0 )
		return false;
	if ( INITRD_S_ISDIR(inode->mode) )
		if ( !ExtractDir(ctx, inode, node) )
			return false;
	if ( INITRD_S_ISREG(inode->mode) )
		if ( !ExtractFile(ctx, inode, node) )
			return false;
	struct timespec ctime = timespec_make((time_t) inode->ctime, 0);
	struct timespec mtime = timespec_make((time_t) inode->mtime, 0);
	if ( node->utimens(&ctx->ioctx, &mtime, &ctime, &mtime) < 0 )
		return false;
	return true;
}

bool ExtractFromPhysicalInto(addr_t physaddr, size_t size, Ref<Descriptor> desc)
{
	initrd_activity("      ", "Verifying ramdisk checksum");

	// Allocate the needed kernel virtual address space.
	addralloc_t initrd_addr_alloc;
	if ( !AllocateKernelAddress(&initrd_addr_alloc, size) )
		PanicF("Can't allocate 0x%zx bytes of kernel address space for the "
		        "init ramdisk", size );

	// Map the physical frames onto our address space.
	addr_t mapat = initrd_addr_alloc.from;
	for ( size_t i = 0; i < size; i += Page::Size() )
		if ( !Memory::Map(physaddr + i, mapat + i, PROT_KREAD) )
			PanicF("Unable to map the init ramdisk into virtual memory");
	Memory::Flush();

	struct initrd_context ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.initrd = (uint8_t*) mapat;
	ctx.initrd_size = size;
	ctx.amount_extracted = 0;
	ctx.last_percent = -1;
	SetupKernelIOCtx(&ctx.ioctx);

	if ( size < sizeof(*ctx.sb) )
		PanicF("initrd is too small");
	ctx.sb = (initrd_superblock_t*) ctx.initrd;

	if ( !String::StartsWith(ctx.sb->magic, "sortix-initrd") )
		PanicF("Invalid magic value in initrd. This means the ramdisk may have "
		       "been corrupted by the bootloader, or that an incompatible file "
		       "has been passed to the kernel.");

	if ( strcmp(ctx.sb->magic, "sortix-initrd-1") == 0 )
		PanicF("Sortix initrd format version 1 is no longer supported.");

	if ( strcmp(ctx.sb->magic, "sortix-initrd-2") != 0 )
		PanicF("The initrd has a format that isn't supported. Perhaps it is "
		       "too new? Try downgrade or regenerate the initrd.");

	if ( size < ctx.sb->fssize )
		PanicF("The initrd said it is %u bytes, but the kernel was only passed "
		       "%zu bytes by the bootloader, which is not enough.",
		       ctx.sb->fssize, size);

	uint32_t amount = ctx.sb->fssize - ctx.sb->sumsize;
	uint8_t* filesum = ctx.initrd + amount;
	if ( ctx.sb->sumalgorithm != INITRD_ALGO_CRC32 )
	{
		Log::PrintF("Warning: InitRD checksum algorithm not supported\n");
	}
	else
	{
		uint32_t crc32 = *((uint32_t*) filesum);
		uint32_t filecrc32 = CRC32::Hash(ctx.initrd, amount);
		if ( crc32 != filecrc32 )
		{
			PanicF("InitRD had checksum %X, expected %X: this means the ramdisk "
				   "may have been corrupted by the bootloader.", filecrc32, crc32);
		}
	}

	initrd_activity("  OK  ", "Verifying ramdisk checksum");

	initrd_progress(&ctx);

	if ( desc->mkdir(&ctx.ioctx, ".initrd-links", 0777) != 0 )
		return false;

	if ( !(ctx.links = desc->open(&ctx.ioctx, ".initrd-links", O_READ | O_DIRECTORY, 0)) )
		return false;

	if ( !ExtractNode(&ctx, initrd_get_inode(&ctx, ctx.sb->root), desc) )
		return false;

	union
	{
		struct kernel_dirent dirent;
		uint8_t dirent_data[sizeof(struct kernel_dirent) + sizeof(uintmax_t) * 3];
	};

	while ( 0 < ctx.links->readdirents(&ctx.ioctx, &dirent, sizeof(dirent_data), 1) &&
	        ((const char*) dirent.d_name)[0] )
	{
		if ( ((const char*) dirent.d_name)[0] == '.' )
			continue;
		ctx.links->unlink(&ctx.ioctx, dirent.d_name);
		ctx.links->lseek(&ctx.ioctx, 0, SEEK_SET);
	}

	ctx.links.Reset();

	desc->rmdir(&ctx.ioctx, ".initrd-links");

	// Unmap the pages and return the physical frames for reallocation.
	for ( size_t i = 0; i < initrd_addr_alloc.size; i += Page::Size() )
	{
		if ( !Memory::LookUp(mapat + i, NULL, NULL) )
			continue;
		addr_t addr = Memory::Unmap(mapat + i);
		Page::Put(addr, PAGE_USAGE_WASNT_ALLOCATED);
	}
	Memory::Flush();

	// Free the used virtual address space.
	FreeKernelAddress(&initrd_addr_alloc);

	ctx.amount_extracted = ctx.initrd_size;
	initrd_progress(&ctx);
	initrd_activity_done();

	return true;
}

} // namespace InitRD
} // namespace Sortix
