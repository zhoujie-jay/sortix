/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * unistd/execvpe.c
 * Loads a program image.
 */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: Move this to some generic environment interface.
static const char* LookupEnvironment(const char* name, char* const* envp)
{
	size_t equalpos = strcspn(name, "=");
	if ( name[equalpos] == '=' )
		return NULL;
	size_t namelen = equalpos;
	for ( size_t i = 0; envp[i]; i++ )
	{
		if ( strncmp(name, envp[i], namelen) )
			continue;
		if ( envp[i][namelen] != '=' )
			continue;
		return envp[i] + namelen + 1;
	}
	return NULL;
}

static
int execvpe_attempt(const char* filename,
                    const char* original,
                    char* const* argv,
                    char* const* envp)
{
	execve(filename, argv, envp);

	if ( errno != ENOEXEC )
		return -1;

	// Prevent attempting to run the shell using itself in an endless loop if it
	// happens to be an unknown format or a shell script itself.
	if ( !strcmp(original, "sh") )
		return errno = ENOEXEC, -1;

	int argc = 0;
	while ( argv[argc] )
		argc++;

	if ( INT_MAX - argc < 1 + 1 )
		return errno = EOVERFLOW, -1;

	int new_argc = 1 + argc;
	char** new_argv = (char**) malloc(sizeof(char*) * (new_argc + 1));
	if ( !new_argv )
		return -1;

	new_argv[0] = (char*) "sh";
	new_argv[1] = (char*) filename;
	for ( int i = 1; i < argc; i++ )
		new_argv[1 + i] = argv[i];
	new_argv[new_argc] = (char*) NULL;

	execvpe(new_argv[0], new_argv, envp);

	free(new_argv);

	return errno = ENOEXEC, -1;
}

// NOTE: The PATH-searching logic is repeated multiple places. Until this logic
//       can be shared somehow, you need to keep this comment in sync as well
//       as the logic in these files:
//         * kernel/process.cpp
//         * libc/unistd/execvpe.c
//         * utils/which.c

int execvpe(const char* filename, char* const* argv, char* const* envp)
{
	if ( !filename || !filename[0] )
		return errno = ENOENT;

	const char* path = LookupEnvironment("PATH", envp);
	bool search_path = !strchr(filename, '/') && path;
	bool any_tries = false;
	bool any_eacces = false;

	// Search each directory in the PATH variable for a suitable file.
	while ( search_path && *path )
	{
		size_t len = strcspn(path, ":");
		if ( !len )
		{
			// Sortix doesn't support that the empty string means current
			// directory. While it does inductively make sense, the common
			// kernel interfaces such as openat doesn't accept it and software
			// often just prefix their directories and a colon to PATH without
			// regard to whether it's already empty.
			path++;
			continue;
		}

		any_tries = true;

		// Determine the directory prefix.
		char* dirpath = strndup(path, len);
		if ( !dirpath )
			return -1;
		if ( (path += len)[0] == ':' )
			path++;
		while ( len && dirpath[len - 1] == '/' )
			dirpath[--len] = '\0';

		// Determine the full path to the file inside the directory.
		char* fullpath;
		if ( asprintf(&fullpath, "%s/%s", dirpath, filename) < 0 )
			return free(dirpath), -1;

		execvpe_attempt(fullpath, filename, argv, envp);

		free(fullpath);
		free(dirpath);

		// Proceed to the next PATH entry if the file didn't exist.
		if ( errno == ENOENT )
			continue;

		// Ignore errors related to path resolution where the cause is a bad
		// entry in the PATH as opposed to security issues.
		if ( errno == ELOOP ||
		     errno == EISDIR ||
		     errno == ENAMETOOLONG ||
		     errno == ENOTDIR )
			continue;

		// Remember permission denied errors and report that errno value if the
		// entire PATH search fails rather than the error of the last attempt.
		if ( errno == EACCES )
		{
			any_eacces = true;
			continue;
		}

		// Any other errors are treated as fatal and we stop the search.
		break;
	}

	if ( !any_tries )
		execvpe_attempt(filename, filename, argv, envp);

	if ( any_eacces )
		errno = EACCES;

	return -1;
}
