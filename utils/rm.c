/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * rm.c
 * Remove files or directories.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: Wrong implementation of -f! It means ignore *nonexistent files*, not
// ignore all errors!

int DupHandleCWD(int fd)
{
	if ( fd == AT_FDCWD )
		return open(".", O_RDONLY);
	return dup(fd);
}

bool RemoveRecursively(int dirfd, const char* full, const char* rel,
                       bool force, bool verbose)
{
	if ( unlinkat(dirfd, rel, 0) == 0 )
	{
		if ( verbose )
			fprintf(stderr, "removed `%s'\n", full);
		return true;
	}
	if ( errno != EISDIR )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot remove: %s", full);
		return false;
	}
	if ( unlinkat(dirfd, rel, AT_REMOVEDIR) == 0 )
	{
		if ( verbose )
			fprintf(stderr, "removed `%s'\n", full);
		return true;
	}
	if ( errno != ENOTEMPTY )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot remove: %s", full);
		return false;
	}
	int targetfd = openat(dirfd, rel, O_RDONLY | O_DIRECTORY);
	if ( targetfd < 0 )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot open: %s", full);
		return false;
	}
	DIR* dir = fdopendir(targetfd);
	if ( !dir )
	{
		error(0, errno, "fdopendir");
		close(targetfd);
		return false;
	}
	bool ret = true;
	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		const char* name = entry->d_name;
		if ( strcmp(name, ".") == 0 || strcmp(name, "..") == 0 )
			continue;
		char* newfull = (char*) malloc(strlen(full) + 1 + strlen(entry->d_name) + 1);
		bool addslash = !full[0] || full[strlen(full)-1] != '/';
		stpcpy(stpcpy(stpcpy(newfull, full), addslash ? "/" : ""), name);
		bool ok = RemoveRecursively(targetfd, newfull, name, false, verbose);
		free(newfull);
		rewinddir(dir);
		if ( !ok ) { ret = false; break; }
	}
	closedir(dir);
	if ( ret )
	{
		if ( unlinkat(dirfd, rel, AT_REMOVEDIR) == 0 )
		{
			if ( verbose )
				fprintf(stderr, "removed `%s'\n", full);
		}
		else
		{
			if ( !force || errno == ENOENT )
			{
				error(0, errno, "cannot remove: %s", full);
				ret = false;
			}
		}
	}
	return ret;
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... FILE...\n", argv0);
	fprintf(fp, "Remove files by unlinking their directory entries.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool force = false;
	bool recursive = false;
	bool verbose = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'f': force = true; break;
			case 'r': recursive = true; break;
			case 'R': recursive = true; break;
			case 'v': verbose = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--force") )
			force = true;
		else if ( !strcmp(arg, "--recursive") )
			recursive = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( argc < 2 && !force )
		error(1, 0, "missing operand");

	int main_ret = 0;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( unlink(arg) < 0 )
		{
			if ( !recursive || errno != EISDIR )
			{
				if ( force && errno == ENOENT )
					continue;
				error(0, errno, "cannot remove: %s", arg);
				main_ret = 1;
				continue;
			}
			if ( !RemoveRecursively(AT_FDCWD, arg, arg, force, verbose) )
			{
				main_ret = 1;
				continue;
			}
		}
		else
		{
			if ( verbose )
				fprintf(stderr, "removed `%s'\n", arg);
		}
	}

	return main_ret;
}
