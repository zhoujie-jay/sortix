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

	mkinitrd.cpp
	Produces a simple ramdisk meant for bootstrapping the Sortix kernel.

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

void usage(int argc, char* argv[])
{
	printf("usage: %s [OPTIONS] <files>\n", argv[0]);
	printf("Options:\n");
	printf("  -o <file>                Write the ramdisk to this file\n");
	printf("  -q                       Surpress normal output\n");
	printf("  -v                       Be verbose\n");
	printf("  --usage                  Display this screen\n");
	printf("  --help                   Display this screen\n");
	printf("  --version                Display version information\n");
}

void version()
{
	printf("mkinitrd 0.1\n");
	printf("Copyright (C) 2011 Jonas 'Sortie' Termansen\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("website: http://www.maxsi.org/software/sortix/\n");
}

bool verbose = false;

int main(int argc, char* argv[])
{
	const char* dest = NULL;

	if ( argc < 2 ) { usage(argc, argv); return 0; }

	uint32_t numfiles = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp(argv[i], "-o") == 0 )
		{
			if ( i + 1 < argc )
			{
				dest = argv[i+1];
				argv[i+1] = NULL;
			}
			argv[i] = NULL;
			i++;
		}
		else if ( strcmp(argv[i], "-q") == 0 )
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
		else
		{
			numfiles++;
		}
	}

	if ( dest == NULL )
	{
		fprintf(stderr, "%s: no output file specified\n", argv[0]);
		return 0;
	}

	int fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if ( fd < 0 )
	{
		error(0, errno, "%s", dest);
		return 1;
	}

	// Write the initrd headers.
	Sortix::InitRD::Header header;
	memset(&header, 0, sizeof(header));
	strcpy(header.magic, "sortix-initrd-1");
	header.numfiles = numfiles;
	if ( !writeall(fd, &header, sizeof(header)) )
	{
		error(0, errno, "write: %s", dest);
		close(fd);
		unlink(dest);
		return 1;
	}

	uint32_t fileoffset = sizeof(header) + numfiles * sizeof(Sortix::InitRD::FileHeader);

	uint32_t filenum = 0;
	for ( int i = 1; i < argc; i++ )
	{
		const char* file = argv[i];

		if ( file == NULL ) { continue; }

		Sortix::InitRD::FileHeader fileheader;
		memset(&fileheader, 0, sizeof(fileheader));

		if ( sizeof(fileheader.name) - 1 < strlen(file) )
		{
			fprintf(stderr, "%s: file name is too long: %s\n", argv[0], file);
			close(fd);
			unlink(dest);
			return 1;
		}

		int filefd = open(file, O_RDONLY);
		if ( filefd < 0 )
		{
			error(0, errno, "%s", file);
			close(fd);
			unlink(dest);
			return 1;
		}

		if ( verbose )
		{
			fprintf(stderr, "%s\n", file);
		}

		struct stat st;
		if ( fstat(filefd, &st) != 0 )
		{
			error(0, errno, "stat: %s", file);
			close(fd);
			unlink(dest);
			return 1;
		}

		uint32_t filesize = st.st_size;
		fileheader.permissions = 0755;
		fileheader.size = filesize;
		fileheader.owner = 1;
		fileheader.group = 1;
		fileheader.offset = fileoffset;
		strcpy(fileheader.name, file);

		off_t fileheaderpos = sizeof(header) + filenum * sizeof(Sortix::InitRD::FileHeader);
		if ( lseek(fd, fileheaderpos, SEEK_SET ) < 0 )
		{
			error(0, errno, "seek: %s", dest);
			close(fd);
			unlink(dest);
			return 1;
		}

		if ( !writeall(fd, &fileheader, sizeof(fileheader)) )
		{
			error(0, errno, "write: %s", dest);
			close(fd);
			unlink(dest);
			return 1;
		}

		if ( lseek(fd, fileoffset, SEEK_SET ) < 0 )
		{
			error(0, errno, "seek: %s", dest);
			close(fd);
			unlink(dest);
			return 1;
		}

		const size_t BUFFER_SIZE = 16384UL;
		uint8_t buffer[BUFFER_SIZE];

		uint32_t readsofar = 0;
		while ( readsofar < filesize )
		{
			uint32_t left = filesize-readsofar;
			size_t toread = (left < BUFFER_SIZE) ? left : BUFFER_SIZE;
			ssize_t bytesread = read(filefd, buffer, toread);
			if ( bytesread <= 0 )
			{
				error(0, errno, "read: %s", file);
				close(fd);
				unlink(dest);
				return 1;
			}

			if ( !writeall(fd, buffer, bytesread) )
			{
				error(0, errno, "write: %s", dest);
				close(fd);
				unlink(dest);
				return 1;
			}

			readsofar += bytesread;
		}

		fileoffset += filesize;
		filenum++;

		close(filefd);
	}

	return 0;
}
