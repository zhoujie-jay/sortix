/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	lsinitrd.cpp
	Lists the files in a Sortix ramdisk.

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <sortix/initrd.h>

bool readall(int fd, void* p, size_t size)
{
	uint8_t* buffer = (uint8_t*) p;

	size_t bytesread = 0;

	while ( bytesread < size )
	{
		ssize_t justread = read(fd, buffer + bytesread, size - bytesread);
		if ( justread <= 0 ) { return false; }
		bytesread += justread;
	}

	return true;
}

void usage(int argc, char* argv[])
{
	printf("usage: %s [OPTIONS] <ramdisks>\n", argv[0]);
	printf("Options:\n");
	printf("  -q                       Surpress normal output\n");
	printf("  -v                       Be verbose\n");
	printf("  --usage                  Display this screen\n");
	printf("  --help                   Display this screen\n");
	printf("  --version                Display version information\n");
}

void version()
{
	printf("lsinitrd 0.1\n");
	printf("Copyright (C) 2011 Jonas 'Sortie' Termansen\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("website: http://www.maxsi.org/software/sortix/\n");
}

bool verbose = false;

int listfiles(const char* filepath)
{
	int fd = open(filepath, O_RDONLY);
	if ( fd < 0 ) { error(0, errno, "%s", filepath); return 1; }

	Sortix::InitRD::Header header;
	if ( !readall(fd, &header, sizeof(header)) )
	{
		error(0, errno, "read: %s", filepath);
		close(fd);
		return 1;
	}

	if ( strcmp(header.magic, "sortix-initrd-1") != 0 )
	{
		error(0, 0, "not a sortix ramdisk: %s", filepath);
		close(fd);
		return 1;
	}

	for ( uint32_t i = 0; i < header.numfiles; i++ )
	{
		Sortix::InitRD::FileHeader fileheader;
		if ( !readall(fd, &fileheader, sizeof(fileheader)) )
		{
			error(0, errno, "read: %s", filepath);
			close(fd);
			return 1;
		}

		printf("%s\n", fileheader.name);
	}

	close(fd);

	return 0;
}

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { usage(argc, argv); return 0; }

	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp(argv[i], "-q") == 0 )
		{
			verbose = false;
			argv[i] = NULL;
		}
		else if ( strcmp(argv[i], "-v") == 0 ) 
		{
			verbose = true;
			argv[i] = NULL;
		}
		else if ( strcmp(argv[i], "--usage") == 0 ) 
		{
			usage(argc, argv);
			return 0;
		}
		else if ( strcmp(argv[i], "--help") == 0 ) 
		{
			usage(argc, argv);
			return 0;
		}
		else if ( strcmp(argv[i], "--version") == 0 ) 
		{
			version();
			return 0;
		}
	}

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( argv[i] == NULL ) { continue; }

		result |= listfiles(argv[i]);
	}

	return result;
}
