/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * stdlib/canonicalize_file_name_at.c
 * Return the canonicalized filename.
 */

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
		if ( fstatat(dupdirfd, entry->d_name, &st, AT_SYMLINK_NOFOLLOW) )
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

static const char* SkipSlashes(const char* path)
{
	while ( *path == '/' )
		path++;
	return *path ? path : NULL;
}

char* canonicalize_file_name_at(int dirfd, const char* path)
{
	if ( path && path[0] == '.' && !path[1] )
		path = NULL;

	// The below code is able to determine the absolute path to a directory by
	// using the .. entry and searching the parent directory. This doesn't work
	// for files, as they can be linked in any number of directories and have no
	// pointer back to the directory they were opened from. We'll therefore
	// follow the path until the last element, and if the path points to a file
	// we'll simply append that to the final result.
	if ( path )
	{
		// Open the directory specified by the last slash in the path.
		char* last_slash = strrchr(path, '/');
		if ( last_slash )
		{
			size_t subpath_len = (size_t) (last_slash - path + 1);
			char* subpath = (char*) malloc((subpath_len + 1) * sizeof(char));
			if ( !subpath )
				return NULL;
			memcpy(subpath, path, subpath_len * sizeof(char));
			subpath[subpath_len] = '\0';
			int fd = openat(dirfd, subpath, O_RDONLY | O_DIRECTORY);
			free(subpath);
			if ( fd < 0 )
				return NULL;
			path = SkipSlashes(last_slash + 1);
			char* ret = canonicalize_file_name_at(fd, path);
			close(fd);
			return ret;
		}

		// We have reached the final element in the path. It could be:
		// 1) A directory.
		// 2) A file.
		// 3) A symbolic link to a directory.
		// 4) A symbolic link to a file.

		// The ideal case is if it's a directory (case 1). We follow symbolic
		// links to directories as that's also okay (case 3).
		int fd = openat(dirfd, path, O_RDONLY | O_DIRECTORY);
		if ( 0 <= fd )
		{
			char* ret = canonicalize_file_name_at(fd, NULL);
			close(fd);
			return ret;
		}

		// Stop if an error happened other that the target wasn't a directory.
		if ( errno != ENOTDIR )
			return NULL;

		// TODO: Symbolic link support here.
		// Okay, so now we are dealing with either case 2 or case 4. Since
		// Sortix doesn't have current have symbolic links, we'll be lazy and
		// just assume we are dealing with a file. Otherwise, we'd have to open
		// with O_NOFOLLOW and if that fails, use readlink to figure out where
		// the symbolic link is going, open the directory that contains it and
		// continue this function from there - or something.
	}

	// Our task is now to determine the absolute path of the directory opened as
	// dirfd and append the path variable to it, if any. We'll find the inode
	// information of the directory and search its parent directory for an entry
	// that resolves to this directory. That'll give us one part of the path.
	// We'll then apply that techinique recursively until we hit a directory
	// whose parent is itself (the root directory).

	int fd;
	int parent;
	struct stat fdst;
	struct stat parentst;
	size_t retlen = 0;
	size_t newretlen;
	char* ret = NULL;
	char* newret;
	char* elem;

	if ( path )
	{
		retlen = 1 + strlen(path);
		if ( !(ret = (char*) malloc(sizeof(char) * (retlen + 1))) )
			return NULL;
		stpcpy(stpcpy(ret, "/"), path);
	}
	if ( (fd = dup_handles_cwd(dirfd)) < 0 )
		goto cleanup_done;
	if ( fstat(fd, &fdst) )
		goto cleanup_fd;
	if ( !S_ISDIR(fdst.st_mode) )
	{
		errno = ENOTDIR;
		goto cleanup_fd;
	}
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
