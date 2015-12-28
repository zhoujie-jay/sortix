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

    rmdir.cpp
    Remove an empty directory.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <error.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
	fprintf(fp, "Usage: %s [OPTION]... DIRECTORY...\n", argv0);
	fprintf(fp, "Remove directories if they are empty.\n");
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
	bool ignore_non_empty = false;
	bool parents = false;
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
			while ( char c = *++arg ) switch ( c )
			{
			case 'p': parents = true; break;
			case 'v': verbose = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--ignore-fail-on-non-empty") )
			ignore_non_empty = true;
		else if ( !strcmp(arg, "--parents") )
			parents = true;
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

	if ( argc < 2 )
		error(1, 0, "missing operand");

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		// TODO: Support --parents option.
		(void) parents;
		int ret = rmdir(arg);
		if ( ret < 0 )
		{
			if ( ignore_non_empty && errno == ENOTEMPTY )
			{
				if ( verbose )
					fprintf(stderr, "%s: skipping non-empty directory `%s'\n", argv0, arg);
				continue;
			}
			error(1, errno, "cannot remove directory `%s'", arg);
		}

		if ( verbose )
			fprintf(stderr, "%s: removed directory `%s'\n", argv0, arg);
	}

	return 0;
}
