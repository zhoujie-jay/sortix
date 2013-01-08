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

    ln.cpp
    Create a hard or symbolic link.

*******************************************************************************/

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

void Usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... TARGET LINK_NAME\n", argv0);
	fprintf(fp, "Create a hard or symbolic link.\n");
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
	bool force = false;
	bool symbolic = false;
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
				case 'f': force = true; break;
				case 's': symbolic = true; break;
				case 'v': verbose = true; break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
					Usage(stderr, argv0);
					exit(1);
				}
		else if ( !strcmp(arg, "--force") )
			force = true;
		else if ( !strcmp(arg, "--symbolic") )
			symbolic = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
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

	for ( int i = 0; i < argc; i++ )
		while ( i < argc && !argv[i] )
		{
			for ( int n = i; n < argc; n++ )
				argv[n] = argv[n+1];
			argc--;
		}

	if ( argc != 3 )
	{
		const char* what = argc < 3 ? "missing" : "extra";
		fprintf(stderr, "%s: %s operand\n", argv0, what);
		Usage(stderr, argv0);
		exit(1);
	}

	const char* oldname = argv[1];
	const char* newname = argv[2];

	if ( force )
		unlink(newname);

	if ( symbolic )
		fprintf(stderr, "%s: symbolic links are not supported, creating hard "
		                "link instead\n", argv0);

	int ret = link(oldname, newname);
	if ( ret == 0  )
	{
		if ( verbose )
			printf("`%s' => `%s'\n", newname, oldname);
	}
	else
		error(0, errno, "`%s' => `%s'", newname, oldname);
	return ret ? 1 : 0;
}
