/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * find.c
 * Locate files and directories.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* AddElemToPath(const char* path, const char* elem)
{
	size_t pathlen = strlen(path);
	size_t elemlen = strlen(elem);
	if ( pathlen && path[pathlen-1] == '/' )
	{
		char* ret = (char*) malloc(sizeof(char) * (pathlen + elemlen + 1));
		stpcpy(stpcpy(ret, path), elem);
		return ret;
	}
	char* ret = (char*) malloc(sizeof(char) * (pathlen + 1 + elemlen + 1));
	stpcpy(stpcpy(stpcpy(ret, path), "/"), elem);
	return ret;
}

static const int TYPE_FILE = 1 << 0;
static const int TYPE_DIR = 1 << 1;

bool Find(int dirfd, const char* relpath, const char* path, int types)
{
	bool ret = true;
	int fd = openat(dirfd, relpath, O_RDONLY | O_SYMLINK_NOFOLLOW);
	if ( fd < 0 ) { error(0, errno, "%s", path); return false; }
	struct stat st;
	if ( fstat(fd, &st) ) { error(0, errno, "stat: %s", path); return false; }
	if ( S_ISDIR(st.st_mode) )
	{
		if ( types & TYPE_DIR )
			printf("%s\n", path);
		DIR* dir = fdopendir(fd);
		if ( !dir ) { error(0, errno, "fdopendir"); close(fd); return false; }
		struct dirent* entry;
		while ( (entry = readdir(dir)) )
		{
			const char* name = entry->d_name;
			if ( !strcmp(name, ".") || !strcmp(name, "..") )
				continue;
			char* newpath = AddElemToPath(path, name);
			if ( !Find(fd, name, newpath, types) )
			{
				ret = false;
				break;
			}
		}
		closedir(dir);
	}
	else if ( S_ISREG(st.st_mode) )
	{
		if ( types & TYPE_FILE )
			printf("%s\n", path);
	}
	return ret;
}

int main(int argc, char* argv[])
{
	const char* path = NULL;
	bool found_options = false;
	int types = 0;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
		{
			if ( found_options )
				error(1, 0, "path `%s' must come before options", arg);
			if ( path )
				error(1, 0, "multiple paths are not supported");
			path = arg;
		}
		else if ( !strcmp(arg, "-type") )
		{
			if ( i + 1 == argc )
				error(1, 0, "-type expects an argument");
			arg = argv[++i];
			if ( !strcmp(arg, "f") )
				types |= TYPE_FILE;
			else if ( !strcmp(arg, "d") )
				types |= TYPE_DIR;
			else
				error(1, 0, "unknown `-type %s'", arg);
		}
		else
			error(1, 0, "unknown option `%s'", arg);
	}
	if ( !path )
		path = ".";
	if ( !types )
		types = TYPE_FILE | TYPE_DIR;
	return Find(AT_FDCWD, path, path, types) ? 0 : 1;
}
