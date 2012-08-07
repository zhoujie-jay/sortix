/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    getcwd.cpp
    Returns the current working directory.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int dup_handles_cwd(int fd)
{
	if ( fd == AT_FDCWD )
		return open(".", O_RDONLY | O_DIRECTORY);
	return dup(fd);
}

static char* FindDirectoryEntryAt(int dirfd, ino_t inode, dev_t dev)
{
	int dupdirfd = dup_handles_cwd(dirfd);
	if ( dupdirfd < 0 )
		return NULL;
	DIR* dir = fdopendir(dupdirfd);
	if ( !dir ) { close(dupdirfd); return NULL; }
	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		if ( !strcmp(entry->d_name, "..") )
			continue;
		struct stat st;
		if ( fstatat(dupdirfd, entry->d_name, &st, 0) )
			continue; // Not ideal, but missing permissions, broken symlinks..
		if ( st.st_ino == inode && st.st_dev == dev )
		{
			char* result = strdup(entry->d_name);
			closedir(dir);
			return result;
		}
	}
	closedir(dir);
	errno = ENOENT;
	return NULL;
}

extern "C" char* get_current_dir_name(void)
{
	int fd;
	int parent;
	struct stat fdst;
	struct stat parentst;
	size_t retlen = 0;
	size_t newretlen;
	char* ret = NULL;
	char* newret;
	char* elem;

	fd = open(".", O_RDONLY | O_DIRECTORY);
	if ( fd < 0 )
		goto cleanup_done;
	if ( fstat(fd, &fdst) )
		goto cleanup_fd;
next_parent:
	parent = openat(fd, "..", O_RDONLY | O_DIRECTORY);
	if ( parent < 0 )
		goto cleanup_fd;
	if ( fstat(parent, &parentst) )
		goto cleanup_parent;
	if ( fdst.st_ino == parentst.st_ino &&
	     fdst.st_dev == parentst.st_dev )
	{
		close(fd);
		close(parent);
		return ret ? ret : strdup("/");
	}
	elem = FindDirectoryEntryAt(parent, fdst.st_ino, fdst.st_dev);
	if ( !elem )
		goto cleanup_parent;
	newretlen = 1 + strlen(elem) + retlen;
	newret = (char*) malloc(sizeof(char) * (newretlen + 1));
	if ( !newret )
		goto cleanup_elem;
	stpcpy(stpcpy(stpcpy(newret, "/"), elem), ret ? ret : "");
	free(elem);
	free(ret);
	ret = newret;
	retlen = newretlen;
	close(fd);
	fd = parent;
	fdst = parentst;
	goto next_parent;

cleanup_elem:
	free(elem);
cleanup_parent:
	close(parent);
cleanup_fd:
	close(fd);
cleanup_done:
	free(ret);
	return NULL;
}

extern "C" char* getcwd(char* buf, size_t size)
{
	char* cwd = get_current_dir_name();
	if ( !buf )
		return cwd;
	if ( !cwd )
		return NULL;
	if ( size < strlen(cwd)+1 ) { free(cwd); errno = ERANGE; return NULL; }
	strcpy(buf, cwd);
	free(cwd);
	return buf;
}
