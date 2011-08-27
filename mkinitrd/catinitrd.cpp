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

	catinitrd.cpp
	Output files in a Sortix ramdisk.

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

bool writeall(int fd, const void* p, size_t size)
{
	const uint8_t* buffer = (const uint8_t*) p;

	size_t bytesWritten = 0;

	while ( bytesWritten < size )
	{
		ssize_t written = write(fd, buffer + bytesWritten, size - bytesWritten);
		if ( written < 0 ) { return false; }
		bytesWritten += written;
	}

	return true;
}

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
	printf("usage: %s [OPTIONS] <ramdisk> [files]\n", argv[0]);
	printf("Options:\n");
	printf("  -q                       Surpress normal output\n");
	printf("  -v                       Be verbose\n");
	printf("  --usage                  Display this screen\n");
	printf("  --help                   Display this screen\n");
	printf("  --version                Display version information\n");
}

void version()
{
	printf("catinitrd 0.1\n");
	printf("Copyright (C) 2011 Jonas 'Sortie' Termansen\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("website: http://www.maxsi.org/software/sortix/\n");
}

bool verbose = false;

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { usage(argc, argv); return 0; }

	const char* src = NULL;

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
		else if ( src == NULL )
		{
			src = argv[i];
			argv[i] = NULL;
		}
	}

	int fd = open(src, O_RDONLY);
	if ( fd < 0 ) { error(0, errno, "%s", src); return 1; }

	Sortix::InitRD::Header header;
	if ( !readall(fd, &header, sizeof(header)) )
	{
		error(0, errno, "read: %s", src);
		close(fd);
		return 1;
	}

	if ( strcmp(header.magic, "sortix-initrd-1") != 0 )
	{
		error(0, 0, "not a sortix ramdisk: %s", src);
		close(fd);
		return 1;
	}

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( argv[i] == NULL ) { continue; }

		bool found = false;

		for ( uint32_t n = 0; n < header.numfiles; n++ )
		{
			off_t fileheaderpos = sizeof(header) + n * sizeof(Sortix::InitRD::FileHeader);
			if ( lseek(fd, fileheaderpos, SEEK_SET ) < 0 )
			{
				error(0, errno, "seek: %s", src);
				close(fd);
				return 1;
			}

			Sortix::InitRD::FileHeader fileheader;
			if ( !readall(fd, &fileheader, sizeof(fileheader)) )
			{
				error(0, errno, "read: %s", src);
				close(fd);
				return 1;
			}

			if ( strcmp(argv[i], fileheader.name) != 0 ) { continue; }

			found = true;

			if ( lseek(fd, fileheader.offset, SEEK_SET ) < 0 )
			{
				error(0, errno, "seek: %s", src);
				close(fd);
				return 1;
			}

			const size_t BUFFER_SIZE = 16384UL;
			uint8_t buffer[BUFFER_SIZE];
			
			uint32_t filesize = fileheader.size;
			uint32_t readsofar = 0;
			while ( readsofar < filesize )
			{
				uint32_t left = filesize-readsofar;
				size_t toread = (left < BUFFER_SIZE) ? left : BUFFER_SIZE;
				ssize_t bytesread = read(fd, buffer, toread);
				if ( bytesread <= 0 )
				{
					error(0, errno, "read: %s", src);
					close(fd);
					return 1;
				}

				if ( !writeall(1, buffer, bytesread) )
				{
					error(0, errno, "write: <stdout>");
					close(fd);
					return 1;
				}

				readsofar += bytesread;
			}
		}

		if ( !found )
		{
			result |= 1;
			error(0, ENOENT, "%s", argv[i]);
		}
	}

	close(fd);

	return result;
}
