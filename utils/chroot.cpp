/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    chroot.cpp
    Runs a process with another root directory.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

void CompactArguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	while ( i < *argc && !(*argv)[i] )
	{
		for ( int n = i; n < *argc; n++ )
			(*argv)[n] = (*argv)[n+1];
		(*argc)--;
	}
}

void Usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... ROOT [CMD] [ARGUMENT...]\n", argv0);
}

void Help(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
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
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				Usage(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--usage") ) { Usage(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			Usage(stderr, argv0);
			exit(1);
		}
	}

	CompactArguments(&argc, &argv);

	if ( argc < 2 )
	{
		error(0, 0, "missing operand, expected new root directory");
		Usage(stdout, argv0);
		exit(0);
	}

	if ( chroot(argv[1]) != 0 )
		error(1, errno, "`%s'", argv[1]);

	if ( chdir("/.") != 0 )
		error(1, errno, "chdir: `%s/.'", argv[1]);

	char* default_argv[] = { (char*) "sh", (char*) NULL };

	char** exec_argv = 3 <= argc ? argv + 2 : default_argv;
	execvp(exec_argv[0], exec_argv);

	error(127, errno, "`%s'", exec_argv[0]);
}
