/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    which.cpp
    Locate a program in the PATH.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

// NOTE: The PATH-searching logic is repeated multiple places. Until this logic
//       can be shared somehow, you need to keep this comment in sync as well
//       as the logic in these files:
//         * kernel/process.cpp
//         * libc/unistd/execvpe.cpp
//         * utils/which.cpp
// NOTO: See comments in execvpe() for algorithmic commentary.

bool Which(const char* filename, const char* path, bool all)
{
	bool found = false;

	bool search_path = !strchr(filename, '/') && path;
	bool any_tries = false;
	bool any_eacces = false;

	while ( search_path && *path )
	{
		size_t len = strcspn(path, ":");
		if ( !len )
		{
			path++;
			continue;
		}

		any_tries = true;

		char* dirpath = strndup(path, len);
		if ( !dirpath )
			return -1;
		if ( (path += len)[0] == ':' )
			path++;
		while ( len && dirpath[len - 1] == '/' )
			dirpath[--len] = '\0';

		char* fullpath;
		if ( asprintf(&fullpath, "%s/%s", dirpath, filename) < 0 )
			return free(dirpath), -1;

		if ( access(fullpath, X_OK) == 0 )
		{
			found = true;
			printf("%s\n", fullpath);
			free(fullpath);
			free(dirpath);
			if ( all )
				continue;
			break;
		}

		free(fullpath);
		free(dirpath);

		if ( errno == ENOENT )
			continue;

		if ( errno == ELOOP ||
		     errno == EISDIR ||
		     errno == ENAMETOOLONG ||
		     errno == ENOTDIR )
			continue;

		if ( errno == EACCES )
		{
			any_eacces = true;
			continue;
		}

		if ( all )
			continue;

		break;
	}

	if ( !any_tries )
	{
		if ( access(filename, X_OK) == 0 )
		{
			found = true;
			printf("%s\n", filename);
		}
	}

	(void) any_eacces;

	return found;
}

void Help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [-a] FILENAME...\n", argv0);
	fprintf(fp, "Locate a program in the PATH.\n");
}

void Version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool all = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
			for ( size_t i = 1; arg[i]; i++ )
				switch ( arg[i] )
				{
				case 'a': all = true; break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
					Help(stderr, argv0);
					exit(1);
				}
		else if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			Help(stderr, argv0);
			exit(1);
		}
	}

	int num_args = 0;
	for ( int i = 1; i < argc; i++ )
		if ( argv[i] )
			num_args++;

	if ( !num_args )
	{
		fprintf(stderr, "%s: missing operand\n", argv0);
		exit(1);
	}

	const char* path = getenv("PATH");

	bool success = true;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg )
			continue;
		if ( !Which(arg, path, all) )
			success = false;
	}

	return success ? 0 : 1;
}
