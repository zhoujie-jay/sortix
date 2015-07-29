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

// NOTE: The PATH-searching logic is repeated multiple places. Until this logic
//       can be shared somehow, you need to keep this comment in sync as well
//       as the logic in these files:
//         * kernel/process.cpp
//         * libc/unistd/execvpe.cpp
//         * utils/which.cpp
// NOTE: See comments in execvpe() for algorithmic commentary.

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
	fprintf(fp, "Usage: %s [-a] FILENAME...\n", argv0);
	fprintf(fp, "Locate a program in the PATH.\n");
}

static void version(FILE* fp, const char* argv0)
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
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'a': all = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
				help(stderr, argv0);
				exit(1);
			}
		}
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

	if ( argc < 2 )
		error(1, 0, "missing operand");

	const char* path = getenv("PATH");

	bool success = true;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !Which(arg, path, all) )
			success = false;
	}

	return success ? 0 : 1;
}
