/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    dirname.c
    Strip last component from file name.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <libgen.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void do_dirname(char* path, bool zero)
{
	const char* name = dirname(path);
	fputs(name, stdout);
	fputc(zero ? '\0' : '\n', stdout);
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
	fprintf(fp, "Usage: %s [OPTION] NAME...\n", argv0);
	fprintf(fp, "Output each NAME with its last non-slash component and trailing slashes\n");
	fprintf(fp, "removed; if NAME contains no /'s, output '.' (meaning the current directory).\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -z, --zero           separate output with NUL rather than newline\n");
	fprintf(fp, "      --help           display this help and exit\n");
	fprintf(fp, "      --version        output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "Examples:\n");
	fprintf(fp, "  %s /usr/bin/          -> \"/usr\"\n", argv0);
	fprintf(fp, "  %s dir1/str dir2/str  -> \"dir1\" followed by \"dir2\"\n", argv0);
	fprintf(fp, "  %s stdio.h            -> \".\"\n", argv0);
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
	setlocale(LC_ALL, "");

	bool zero = false;

	const char* argv0 = argv[0];
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
			case 'z': zero = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--zero") )
			zero = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( argc <= 1 )
	{
		error(0, 0, "missing operand");
		fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
		exit(1);
	}

	for ( int i = 1; i < argc; i++ )
		do_dirname(argv[i], zero);

	if ( fflush(stdout) == EOF )
		return 1;
	if ( ferror(stdout) )
		return 1;

	return 0;
}
