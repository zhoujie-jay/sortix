/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with Sortix. If not, see <http://www.gnu.org/licenses/>.

    mkinitrd.cpp
    Produces a simple ramdisk filesystem readable by the Sortix kernel.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sortix/initrd.h>

#include "crc32.h"
#include "rules.h"

#if !defined(sortix)
__BEGIN_DECLS
size_t preadall(int fd, void* buf, size_t count, off_t off);
size_t preadleast(int fd, void* buf, size_t least, size_t max, off_t off);
size_t pwriteall(int fd, const void* buf, size_t count, off_t off);
size_t pwriteleast(int fd, const void* buf, size_t least, size_t max, off_t off);
size_t readall(int fd, void* buf, size_t count);
size_t readleast(int fd, void* buf, size_t least, size_t max);
size_t writeall(int fd, const void* buf, size_t count);
size_t writeleast(int fd, const void* buf, size_t least, size_t max);
__END_DECLS
#endif

uint32_t HostModeToInitRD(mode_t mode)
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

mode_t InitRDModeToHost(uint32_t mode)
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

struct Node;
struct DirEntry;

struct DirEntry
{
	char* name;
	Node* node;
};

struct Node
{
	char* path;
	uint32_t ino;
	uint32_t nlink;
	size_t direntsused;
	size_t direntslength;
	DirEntry* dirents;
	mode_t mode;
	time_t ctime;
	time_t mtime;
	bool written;
	size_t refcount;
};

void FreeNode(Node* node)
{
	if ( 1 < node->nlink ) { node->nlink--; return; }
	for ( size_t i = 0; i < node->direntsused; i++ )
	{
		DirEntry* entry = node->dirents + i;
		if ( strcmp(entry->name, ".") != 0 && strcmp(entry->name, "..") != 0 )
		{
			if ( --entry->node->refcount == 0 )
				FreeNode(entry->node);
		}
		free(entry->name);
	}
	free(node->dirents);
	free(node->path);
	free(node);
}

struct CacheEntry
{
	ino_t ino;
	dev_t dev;
	Node* node;
};

size_t cacheused = 0;
size_t cachelen = 0;
CacheEntry* cache = NULL;

InclusionRules path_filter;

Node* LookupCache(dev_t dev, ino_t ino)
{
	for ( size_t i = 0; i < cacheused; i++ )
		if ( cache[i].dev == dev && cache[i].ino == ino )
			return cache[i].node;
	return NULL;
}

bool AddToCache(Node* node, dev_t dev, ino_t ino)
{
	if ( cacheused == cachelen )
	{
		size_t newcachelen = cachelen ? 2 * cachelen : 256;
		size_t newcachesize = newcachelen * sizeof(CacheEntry);
		CacheEntry* newcache = (CacheEntry*) realloc(cache, newcachesize);
		if ( !newcache )
			return false;
		cache = newcache;
		cachelen = newcachelen;
	}
	size_t index = cacheused++;
	cache[index].ino = ino;
	cache[index].dev = dev;
	cache[index].node = node;
	return true;
}

Node* RecursiveSearch(const char* real_path, const char* virt_path,
                      uint32_t* ino, Node* parent = NULL)
{
	printf("%s\n", virt_path);

	if ( virt_path[0] == '/' && !virt_path[1] )
		virt_path = "";

	struct stat st;
	if ( lstat(real_path, &st) ) { perror(real_path); return NULL; }

	Node* cached = LookupCache(st.st_dev, st.st_ino);
	if ( cached ) { cached->refcount++; return cached; }

	Node* node = (Node*) calloc(1, sizeof(Node));
	if ( !node ) { return NULL; }

	node->refcount = 1;
	node->mode = st.st_mode;
	node->ino = (*ino)++;
	node->ctime = st.st_ctim.tv_sec;
	node->mtime = st.st_mtim.tv_sec;

	char* real_path_clone = strdup(real_path);
	if ( !real_path_clone ) { perror("strdup"); free(node); return NULL; }

	node->path = real_path_clone;

	if ( !S_ISDIR(st.st_mode) )
	{
		if ( !AddToCache(node, st.st_dev, st.st_ino) )
		{
			free(real_path_clone);
			free(node);
			return NULL;
		}
		return node;
	}

	DIR* dir = opendir(real_path);
	if ( !dir ) { perror(real_path); FreeNode(node); return NULL; }

	size_t real_path_len = strlen(real_path);
	size_t virt_path_len = strlen(virt_path);

	bool successful = true;
	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		size_t namelen = strlen(entry->d_name);

		size_t virt_subpath_len = virt_path_len + 1 + namelen;
		char* virt_subpath = (char*) malloc(virt_subpath_len+1);
		if ( !virt_subpath ) { perror("malloc"); successful = false; break; }
		stpcpy(stpcpy(stpcpy(virt_subpath, virt_path), "/"), entry->d_name);

		if ( strcmp(entry->d_name, ".") != 0 &&
		     strcmp(entry->d_name, "..") != 0 &&
		     !path_filter.IncludesPath(virt_subpath) )
		{
			free(virt_subpath);
			continue;
		}

		size_t real_subpath_len = real_path_len + 1 + namelen;
		char* real_subpath = (char*) malloc(real_subpath_len+1);
		if ( !real_subpath ) { free(virt_subpath);  perror("malloc"); successful = false; break; }
		stpcpy(stpcpy(stpcpy(real_subpath, real_path), "/"), entry->d_name);

		Node* child = NULL;
		if ( !strcmp(entry->d_name, ".") )
			child = node;
		if ( !strcmp(entry->d_name, "..") )
			child = parent ? parent : node;
		if ( !child )
			child = RecursiveSearch(real_subpath, virt_subpath, ino, node);
		free(real_subpath);
		free(virt_subpath);
		if ( !child ) { successful = false; break; }

		if ( node->direntsused == node->direntslength )
		{
			size_t oldlength = node->direntslength;
			size_t newlength = oldlength ? 2 * oldlength : 8;
			size_t newsize = sizeof(DirEntry) * newlength;
			DirEntry* newdirents = (DirEntry*) realloc(node->dirents, newsize);
			if ( !newdirents ) { perror("realloc"); successful = false; break; }
			node->dirents = newdirents;
			node->direntslength = newlength;
		}

		char* nameclone = strdup(entry->d_name);
		if ( !nameclone ) { perror("strdup"); successful = false; break; }

		DirEntry* entry = node->dirents + node->direntsused++;

		entry->name = nameclone;
		entry->node = child;
	}

	closedir(dir);
	if ( !successful || !AddToCache(node, st.st_dev, st.st_ino) )
	{
		FreeNode(node);
		return NULL;
	}
	return node;
}

bool WriteNode(struct initrd_superblock* sb, int fd, const char* outputname,
               Node* node)
{ {
	if ( node->written )
		return true;

	uint32_t filesize = 0;
	uint32_t origfssize = sb->fssize;
	uint32_t dataoff = origfssize;
	uint32_t filestart = dataoff;

	if ( S_ISLNK(node->mode) ) // Symbolic link
	{
		const size_t NAME_SIZE = 1024UL;
		char name[NAME_SIZE];
		ssize_t namelen = readlink(node->path, name, NAME_SIZE);
		if ( namelen < 0 ) { goto ioreadlink; }
		filesize = (uint32_t) namelen;
		if ( pwriteall(fd, name, filesize, dataoff) < filesize ) goto ioread;
		dataoff += filesize;
	}
	else if ( S_ISREG(node->mode) ) // Regular file
	{
		int nodefd = open(node->path, O_RDONLY);
		if ( nodefd < 0 ) { goto ioopen; }
		const size_t BUFFER_SIZE = 16UL * 1024UL;
		uint8_t buffer[BUFFER_SIZE];
		ssize_t amount;
		while ( 0 < (amount = read(nodefd, buffer, BUFFER_SIZE)) )
		{
			if ( pwriteall(fd, buffer, amount, dataoff) < (size_t) amount )
			{
				close(nodefd);
				goto iowrite;
			}
			dataoff += amount;
			filesize += amount;
		}
		close(nodefd);
		if ( amount < 0 ) { goto ioread; }
	}
	else if ( S_ISDIR(node->mode) ) // Directory
	{
		for ( size_t i = 0; i < node->direntsused; i++ )
		{
			DirEntry* entry = node->dirents + i;
			const char* name = entry->name;
			size_t namelen = strlen(entry->name);
			struct initrd_dirent dirent;
			dirent.inode = entry->node->ino;
			dirent.namelen = (uint16_t) namelen;
			dirent.reclen = sizeof(dirent) + dirent.namelen + 1;
			dirent.reclen = (dirent.reclen+3)/4*4; // Align entries.
			size_t entsize = sizeof(dirent);
			ssize_t hdramt = pwriteall(fd, &dirent, entsize, dataoff);
			ssize_t nameamt = pwriteall(fd, name, namelen+1, dataoff + entsize);
			if ( hdramt < (ssize_t) entsize || nameamt < (ssize_t) (namelen+1) )
				goto iowrite;
			size_t padding = dirent.reclen - (entsize + (namelen+1));
			for ( size_t i = 0; i < padding; i++ )
			{
				uint8_t nul = 0;
				if ( pwrite(fd, &nul, 1, dataoff+entsize+namelen+1+i) != 1 )
					goto iowrite;
			}
			filesize += dirent.reclen;
			dataoff += dirent.reclen;
		}
	}

	struct initrd_inode inode;
	inode.mode = HostModeToInitRD(node->mode);
	inode.uid = 1;
	inode.gid = 1;
	inode.nlink = node->nlink;
	inode.ctime = (uint64_t) node->ctime;
	inode.mtime = (uint64_t) node->mtime;
	inode.dataoffset = filestart;
	inode.size = filesize;

	uint32_t inodepos = sb->inodeoffset + node->ino * sb->inodesize;
	uint32_t inodesize = sizeof(inode);
	if ( pwriteall(fd, &inode, inodesize, inodepos) < inodesize ) goto iowrite;

	uint32_t increment = dataoff - origfssize;
	sb->fssize += increment;

	return node->written = true;
}
  ioreadlink:
	error(0, errno, "readlink: %s", node->path);
	return false;
  ioopen:
	error(0, errno, "open: %s", node->path);
	return false;
  ioread:
	error(0, errno, "read: %s", node->path);
	return false;
  iowrite:
	error(0, errno, "write: %s", outputname);
	return false;
}

bool WriteNodeRecursive(struct initrd_superblock* sb, int fd,
                        const char* outputname, Node* node)
{
	if ( !WriteNode(sb, fd, outputname, node) ) { return false; }

	if ( !S_ISDIR(node->mode) ) { return true; }

	for ( size_t i = 0; i < node->direntsused; i++ )
	{
		DirEntry* entry = node->dirents + i;
		const char* name = entry->name;
		Node* child = entry->node;
		if ( !strcmp(name, ".") || !strcmp(name, ".." ) ) { continue; }
		if ( !WriteNodeRecursive(sb, fd, outputname, child) ) { return false; }
	}

	return true;
}

bool Format(const char* outputname, int fd, uint32_t inodecount, Node* root)
{
	struct initrd_superblock sb;
	memset(&sb, 0, sizeof(sb));
	strncpy(sb.magic, "sortix-initrd-2", sizeof(sb.magic));
	sb.revision = 0;
	sb.fssize = sizeof(sb);
	sb.inodesize = sizeof(initrd_inode);
	sb.inodeoffset = sizeof(sb);
	sb.inodecount = inodecount;
	sb.root = root->ino;

	uint32_t inodebytecount = sb.inodesize * sb.inodecount;
	sb.fssize += inodebytecount;

	if ( !WriteNodeRecursive(&sb, fd, outputname, root) ) { return false; }

	uint32_t crcsize = sizeof(uint32_t);
	sb.sumalgorithm = INITRD_ALGO_CRC32;
	sb.sumsize = crcsize;
	sb.fssize += sb.sumsize;

	if ( pwriteall(fd, &sb, sizeof(sb), 0) < sizeof(sb) )
	{
		error(0, errno, "write: %s", outputname);
		return false;
	}

	uint32_t checksize = sb.fssize - sb.sumsize;
	uint32_t crc;
	if ( !CRC32File(&crc, outputname, fd, 0, checksize) ) { return false; }
	if ( pwriteall(fd, &crc, crcsize, checksize) < crcsize ) { return false; }

	return true;
}

bool Format(const char* pathname, uint32_t inodecount, Node* root)
{
	int fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0666);
	bool result = Format(pathname, fd, inodecount, root);
	close(fd);
	return result;
}

void Usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s <ROOT> -o <DEST> [-f <PATH-FILTER>]\n", argv0);
	fprintf(fp, "Creates a init ramdisk for the Sortix kernel.\n");
}

void Help(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
}

void Version(FILE* fp, const char* argv0)
{
	(void) argv0;
	fprintf(fp, "mkinitrd 0.4\n");
	fprintf(fp, "Copyright (C) 2012, 2013 Jonas 'Sortie' Termansen\n");
	fprintf(fp, "This is free software; see the source for copying conditions.  There is NO\n");
	fprintf(fp, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	fprintf(fp, "website: http://www.maxsi.org/software/sortix/\n");
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	if ( argc < 2 ) { Usage(stdout, argv0); exit(0); }
	const char* dest = NULL;
	for ( int i = 1; i < argc; i++ )
	{
		int argsleft = argc - i - 1;
		const char* arg = argv[i];
		if ( arg[0] != '-' ) { continue; }
		argv[i] = NULL;
		if ( !strcmp(arg, "--") ) { break; }
		if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--usage") ) { Usage(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "-o") || !strcmp(arg, "--output") )
		{
			if ( argsleft < 1 )
			{
				fprintf(stderr, "No output file specified\n");
				Usage(stderr, argv0);
				exit(1);
			}
			dest = argv[++i]; argv[i] = NULL;
			continue;
		}
		if ( !strcmp(arg, "-f") || !strcmp(arg, "--filter") )
		{
			if ( argsleft < 1 )
			{
				fprintf(stderr, "No filter rule file specified\n");
				Usage(stderr, argv0);
				exit(1);
			}
			const char* path = argv[++i]; argv[i] = NULL;
			FILE* fp = fopen(path, "r");
			if ( !fp )
				error(1, errno, "%s", path);
			if ( !path_filter.AddRulesFromFile(fp, stderr, path) )
				exit(1);
			fclose(fp);
			continue;
		}
		fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
		Usage(stderr, argv0);
		exit(1);
	}

	const char* rootstr = NULL;
	int args = 0;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !argv[i] ) { continue; }
		args++;
		rootstr = argv[i];
	}

	const char* errmsg = NULL;
	if ( !errmsg && args < 1 ) { errmsg = "no root specified"; }
	if ( !errmsg && 1 < args ) { errmsg = "too many roots"; }
	if ( !errmsg && !dest ) { errmsg = "no destination specified"; }

	if ( errmsg )
	{
		fprintf(stderr, "%s: %s\n", argv0, errmsg),
		Usage(stderr, argv0);
		exit(1);
	}

	uint32_t inodecount = 1;
	Node* root = RecursiveSearch(rootstr, "/", &inodecount);
	if ( !root ) { exit(1); }

	if ( !Format(dest, inodecount, root) ) { exit(1); }

	FreeNode(root);

	return 0;
}
