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

    mkdir.cpp
    Create a directory.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

void CreateDirectory(const char* path, bool parents, bool verbose)
{
	if ( mkdir(path, 0777) < 0 )
	{
		if ( parents )
		{
			if ( errno == EEXIST )
				return;
			if ( errno == ENOENT )
			{
				char* path_copy = strdup(path);
				char* last_slash = strrchr(path_copy, '/');
				if ( last_slash )
				{
					*last_slash = 0;
					if ( *path_copy )
					{
						CreateDirectory(path_copy, parents, verbose);
						free(path_copy);
						CreateDirectory(path, parents, verbose);
						return;
					}
				}
				free(path_copy);
			}
		}
		error(1, errno, "cannot create directory `%s'", path);
	}
	if ( verbose )
		fprintf(stderr, "%s: created directory `%s'\n", program_invocation_name, path);
}

void Help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... DIRECTORY...\n", argv0);
	fprintf(fp, "Create directories if they don't already exist.\n");
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
	bool parents = false;
	bool verbose = false;
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
				case 'p': parents = true; break;
				case 'v': verbose = true; break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
					Help(stderr, argv0);
					exit(1);
				}
		else if ( !strcmp(arg, "--parents") )
			parents = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
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

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg )
			continue;
		CreateDirectory(arg, parents, verbose);
	}
	return 0;
}
