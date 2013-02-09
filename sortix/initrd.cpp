/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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
    Provides low-level access to a Sortix init ramdisk.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/crc32.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/syscall.h>

#include <sortix/fcntl.h>
#include <sortix/initrd.h>
#include <sortix/stat.h>
#include <sortix/mman.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "initrd.h"

namespace Sortix {
namespace InitRD {

addralloc_t initrd_addr_alloc;
uint8_t* initrd = NULL;
size_t initrdsize;
const initrd_superblock_t* sb;

__attribute__((unused))
static uint32_t HostModeToInitRD(mode_t mode)
{
	uint32_t result = mode & 0777; // Lower 9 bits per POSIX and tradition.
	if ( S_ISVTX & mode ) { result |= INITRD_S_ISVTX; }
	if ( S_ISSOCK(mode) ) { result |= INITRD_S_IFSOCK; }
	if ( S_ISLNK(mode) ) { result |= INITRD_S_IFLNK; }
	if ( S_ISREG(mode) ) { result |= INITRD_S_IFREG; }
	if ( S_ISBLK(mode) ) { result |= INITRD_S_IFBLK; }
	if ( S_ISDIR(mode) ) { result |= INITRD_S_IFDIR; }
	if ( S_ISCHR(mode) ) { result |= INITRD_S_IFCHR; }
	if ( S_ISFIFO(mode) ) { result |= INITRD_S_IFIFO; }
	return result;
}

__attribute__((unused))
static mode_t InitRDModeToHost(uint32_t mode)
{
	mode_t result = mode & 0777; // Lower 9 bits per POSIX and tradition.
	if ( INITRD_S_ISVTX & mode ) { result |= S_ISVTX; }
	if ( INITRD_S_ISSOCK(mode) ) { result |= S_IFSOCK; }
	if ( INITRD_S_ISLNK(mode) ) { result |= S_IFLNK; }
	if ( INITRD_S_ISREG(mode) ) { result |= S_IFREG; }
	if ( INITRD_S_ISBLK(mode) ) { result |= S_IFBLK; }
	if ( INITRD_S_ISDIR(mode) ) { result |= S_IFDIR; }
	if ( INITRD_S_ISCHR(mode) ) { result |= S_IFCHR; }
	if ( INITRD_S_ISFIFO(mode) ) { result |= S_IFIFO; }
	return result;
}

uint32_t Root()
{
	return sb->root;
}

static const initrd_inode_t* GetInode(uint32_t inode)
{
	if ( sb->inodecount <= inode ) { errno = EINVAL; return NULL; }
	uint32_t pos = sb->inodeoffset + sb->inodesize * inode;
	return (const initrd_inode_t*) (initrd + pos);
}

bool Stat(uint32_t ino, struct stat* st)
{
	const initrd_inode_t* inode = GetInode(ino);
	if ( !inode ) { return false; }
	st->st_ino = ino;
	st->st_mode = HostModeToInitRD(inode->mode);
	st->st_nlink = inode->nlink;
	st->st_uid = inode->uid;
	st->st_gid = inode->gid;
	st->st_size = inode->size;
	st->st_atime = inode->mtime;
	st->st_ctime = inode->ctime;
	st->st_mtime = inode->mtime;
	st->st_blksize = 1;
	st->st_blocks = inode->size;
	return true;
}

uint8_t* Open(uint32_t ino, size_t* size)
{
	const initrd_inode_t* inode = GetInode(ino);
	if ( !inode ) { return NULL; }
	*size = inode->size;
	return initrd + inode->dataoffset;
}

uint32_t Traverse(uint32_t ino, const char* name)
{
	const initrd_inode_t* inode = GetInode(ino);
	if ( !inode ) { return 0; }
	if ( !INITRD_S_ISDIR(inode->mode) ) { errno = ENOTDIR; return 0; }
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		const initrd_dirent* dirent = (const initrd_dirent*) (initrd + pos);
		if ( dirent->namelen && !strcmp(dirent->name, name) )
		{
			return dirent->inode;
		}
		offset += dirent->reclen;
	}
	errno = ENOENT;
	return 0;
}

const char* GetFilename(uint32_t dir, size_t index)
{
	const initrd_inode_t* inode = GetInode(dir);
	if ( !inode ) { return 0; }
	if ( !INITRD_S_ISDIR(inode->mode) ) { errno = ENOTDIR; return 0; }
	uint32_t offset = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		const initrd_dirent* dirent = (const initrd_dirent*) (initrd + pos);
		if ( index-- == 0 ) { return dirent->name; }
		offset += dirent->reclen;
	}
	errno = EINVAL;
	return NULL;
}

size_t GetNumFiles(uint32_t dir)
{
	const initrd_inode_t* inode = GetInode(dir);
	if ( !inode ) { return 0; }
	if ( !INITRD_S_ISDIR(inode->mode) ) { errno = ENOTDIR; return 0; }
	uint32_t offset = 0;
	size_t numentries = 0;
	while ( offset < inode->size )
	{
		uint32_t pos = inode->dataoffset + offset;
		const initrd_dirent* dirent = (const initrd_dirent*) (initrd + pos);
		numentries++;
		offset += dirent->reclen;
	}
	return numentries;
}

void CheckSum()
{
	uint32_t amount = sb->fssize - sb->sumsize;
	uint8_t* filesum = initrd + amount;
	if ( sb->sumalgorithm != INITRD_ALGO_CRC32 )
	{
		Log::PrintF("Warning: InitRD checksum algorithm not supported\n");
		return;
	}
	uint32_t crc32 = *((uint32_t*) filesum);
	uint32_t filecrc32 = CRC32::Hash(initrd, amount);
	uint32_t doublecheck = CRC32::Hash(initrd, amount);
	if ( filecrc32 != doublecheck )
	{
		// Yes, this has happened. This seems like the goto place for such bugs
		// to trigger, so I added a more accurate warning.
		Panic("Calculating InitRD checksum two times gave different results: "
		      "this likely means the kernel have a corruption bug, possibly "
		      "caused by building libc and kernel with different settings or "
		      "a bug in the scheduler/interrupt handler or who knows. "
		      "It is also possible that the laws of logic has changed.");
	}
	if ( crc32 != filecrc32 )
	{
		PanicF("InitRD had checksum %X, expected %X: this means the ramdisk "
		       "may have been corrupted by the bootloader.", filecrc32, crc32);
	}
}

void Init(addr_t phys, size_t size)
{
	assert(!initrd);

	// Allocate the needed kernel virtual address space.
	if ( !AllocateKernelAddress(&initrd_addr_alloc, size) )
		PanicF("Can't allocate 0x%zx bytes of kernel address space for the "
		        "init ramdisk", size );

	// Map the physical frames onto our address space.
	addr_t mapat = initrd_addr_alloc.from;
	for ( size_t i = 0; i < size; i += Page::Size() )
		if ( !Memory::Map(phys + i, mapat + i, PROT_KREAD) )
			Panic("Unable to map the init ramdisk into virtual memory");
	Memory::Flush();

	initrd = (uint8_t*) mapat;
	initrdsize = size;

	if ( size < sizeof(*sb) ) { PanicF("initrd is too small"); }
	sb = (const initrd_superblock_t*) initrd;

	if ( !String::StartsWith(sb->magic, "sortix-initrd") )
	{
		Panic("Invalid magic value in initrd. This means the ramdisk may have "
		      "been corrupted by the bootloader, or that an incompatible file "
		      "has been passed to the kernel.");
	}

	if ( strcmp(sb->magic, "sortix-initrd-1") == 0 )
	{
		Panic("Sortix initrd format version 1 is no longer supported.");
	}

	if ( strcmp(sb->magic, "sortix-initrd-2") != 0 )
	{
		Panic("The initrd has a format that isn't supported. Perhaps it is "
		      "too new? Try downgrade or regenerate the initrd.");
	}

	if ( size < sb->fssize )
	{
		PanicF("The initrd said it is %u bytes, but the kernel was only passed "
		       "%zu bytes by the bootloader, which is not enough.", sb->fssize,
		       size);
	}

	CheckSum();
}

static bool ExtractDir(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> node);
static bool ExtractFile(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> file);
static bool ExtractNode(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> node);

static bool ExtractDir(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> dir)
{
	size_t numfiles = GetNumFiles(ino);
	for ( size_t i = 0; i < numfiles; i++ )
	{
		const char* name = GetFilename(ino, i);
		if ( !name )
			return false;
		if ( IsDotOrDotDot(name) )
			continue;
		uint32_t childino = Traverse(ino, name);
		if ( !childino )
			return false;
		const initrd_inode_t* child = GetInode(childino);
		mode_t mode = InitRDModeToHost(child->mode);
		if ( INITRD_S_ISDIR(child->mode) )
		{
			if ( dir->mkdir(ctx, name, mode) && errno != EEXIST )
				return false;
			Ref<Descriptor> desc = dir->open(ctx, name, O_RDWR | O_DIRECTORY, 0);
			if ( !desc )
				return false;
			if ( !ExtractNode(ctx, childino, desc) )
				return false;
		}
		if ( INITRD_S_ISREG(child->mode) )
		{
			Ref<Descriptor> desc = dir->open(ctx, name, O_WRONLY | O_CREAT, mode);
			if ( !desc )
				return false;
			if ( !ExtractNode(ctx, childino, desc) )
				return false;
		}
	}
	return true;
}

static bool ExtractFile(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> file)
{
	size_t filesize;
	const uint8_t* data = Open(ino, &filesize);
	if ( !data )
		return false;
	if ( file->truncate(ctx, filesize) != 0 )
		return false;
	size_t sofar = 0;
	while ( sofar < filesize )
	{
		ssize_t numbytes = file->write(ctx, data + sofar, filesize - sofar);
		if ( numbytes <= 0 )
			return false;
		sofar += numbytes;
	}
	return true;
}

static bool ExtractNode(ioctx_t* ctx, uint32_t ino, Ref<Descriptor> node)
{
	const initrd_inode_t* inode = GetInode(ino);
	if ( !inode )
		return false;
	if ( node->chmod(ctx, InitRDModeToHost(inode->mode)) < 0 )
		return false;
	if ( node->chown(ctx, inode->uid, inode->gid) < 0 )
		return false;
	// TODO: utimes.
	if ( INITRD_S_ISDIR(inode->mode) )
		if ( !ExtractDir(ctx, ino, node) )
			return false;
	if ( INITRD_S_ISREG(inode->mode) )
		if ( !ExtractFile(ctx, ino, node) )
			return false;
	return true;
}

bool ExtractInto(Ref<Descriptor> desc)
{
	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	return ExtractNode(&ctx, sb->root, desc);
}

bool ExtractFromPhysicalInto(addr_t physaddr, size_t size, Ref<Descriptor> desc)
{
	Init(physaddr, size);
	return ExtractInto(desc);
}

} // namespace InitRD
} // namespace Sortix
