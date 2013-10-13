/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	cat.cpp
	Concatenate files and print on the standard output.

*******************************************************************************/

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>

int docat(const char* inputname, int fd)
{
	do
	{
		const size_t BUFFER_SIZE = 255;
		char buffer[BUFFER_SIZE+1];
		ssize_t bytesread = read(fd, buffer, BUFFER_SIZE);
		if ( bytesread == 0 ) { break; }
		if ( bytesread < 0 )
		{
			error(0, errno, "read: %s", inputname);
			return 1;
		}
		if ( (ssize_t) writeall(1, buffer, bytesread) < bytesread )
		{
			error(0, errno, "write: %s", inputname);
			return 1;
		}
	} while ( true );
	return 0;
}

void usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "usage: %s [FILE ...]\n", argv0);
	fprintf(fp, "Concatenate files and print on the standard output.\n");
}

void help(FILE* fp, const char* argv0)
{
	return usage(fp, argv0);
}

void version(FILE* fp, const char* argv0)
{
	return usage(fp, argv0);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' ) { continue; }
		argv[i] = NULL;
		if ( !strcmp(arg, "--") ) { break; }
		if ( !strcmp(arg, "--help") ) { help(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--usage") ) { usage(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--version") ) { version(stdout, argv0); exit(0); }
		error(0, 0, "unrecognized option: %s", arg);
		usage(stderr, argv0);
		exit(1);
	}

	int result = 0;

	bool any = false;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !argv[i] ) { continue; }
		any = true;
		int fd = open(argv[i], O_RDONLY);
		if ( fd < 0 )
		{
			error(0, errno, "%s", argv[i]);
			result = 1;
			continue;
		}

		result |= docat(argv[i], fd);
		close(fd);
	}

	if ( !any ) { result = docat("<stdin>", 0); }

	return result;
}
