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

    kernelinfo.cpp
    Prints a kernel information string.

*******************************************************************************/

#include <sys/kernelinfo.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... REQUEST...\n", argv0);
	fprintf(fp, "Prints a kernel information string.\n");
	fprintf(fp, "example: %s name\n", argv0);
	fprintf(fp, "example: %s version\n", argv0);
	fprintf(fp, "example: %s builddate\n", argv0);
	fprintf(fp, "example: %s buildtime\n", argv0);
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}
	size_t bufsize = 32;
	char* buf = (char*) malloc(bufsize);
	if ( !buf )
		error(1, errno, "malloc");
	for ( int i = 1; i < argc; i++ )
	{
retry:
		ssize_t ret = kernelinfo(argv[i], buf, bufsize);
		if ( ret < 0 )
		{
			if ( errno == EINVAL )
			{
				error(1, 0, "%s: No such kernel string", argv[i]);
			}
			error(1, errno, "kernelinfo(\"%s\")", argv[i]);
		}
		if ( ret )
		{
			buf = (char*) realloc(buf, ret);
			if ( !buf )
				error(1, errno, "realloc");
			bufsize = ret;
			goto retry;
		}
		printf("%s\n", buf);
	}
	return 0;
}
