/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

bool Which(const char* cmd, const char* path, bool all)
{
	if ( strchr(cmd, '/') )
	{
		struct stat st;
		if ( stat(path, &st) )
		{
			printf("%s\n", cmd);
			return true;
		}
		return false;
	}

	// Sortix doesn't support that the empty string means current directory.
	bool found = false;
	char* dirname = NULL;
	while ( *path )
	{
		if ( dirname ) { free(dirname); dirname = NULL; }
		size_t len = strcspn(path, ":");
		if ( !len ) { path++; continue; }
		dirname = (char*) malloc((len+1) * sizeof(char));
		if ( !dirname )
			error(1, errno, "malloc");
		memcpy(dirname, path, len * sizeof(char));
		dirname[len] = '\0';
		if ( (path += len)[0] == ':' )
			path++;
		int dirfd = open(dirname, O_RDONLY | O_DIRECTORY);
		if ( dirfd < 0 )
		{
			if ( errno == EACCES )
				error(1, errno, "%s", dirname);
			// TODO: May be a security concern to continue;
			if ( errno == ENOENT )
				continue;
			continue;
		}
		struct stat st;
		int ret = fstatat(dirfd, cmd, &st, 0);
		if ( ret != 0 )
			continue;
		printf("%s/%s\n", dirname, cmd);
		found = true;
		if ( !all )
			break;
	}
	free(dirname);
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
	if ( !path )
	{
		fprintf(stderr, "%s: PATH variable is not set\n", argv0);
		exit(1);
	}

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
