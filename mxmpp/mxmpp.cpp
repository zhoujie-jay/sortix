/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of mxmpp.

	mxmpp is free software: you can redistribute it and/or modify it under
	the terms of the GNU General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	mxmpp is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with mxmpp. If not, see <http://www.gnu.org/licenses/>.

	mxmpp.cpp
	A simple macro preprocessor.

******************************************************************************/

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#ifndef S_IRGRP
#define S_IRGRP (0)
#endif
#ifndef S_IWGRP
#define S_IWGRP (0)
#endif
#ifndef S_IROTH
#define S_IROTH (0)
#endif
#ifndef S_IWOTH
#define S_IWOTH (0)
#endif

#define writeall mxmpp_writeall
bool writeall(int fd, const void* p, size_t size)
{
	const uint8_t* buffer = (const uint8_t*) p;

	size_t bytesWritten = 0;

	while ( bytesWritten < size )
	{
		ssize_t written = write(fd, buffer + bytesWritten, size - bytesWritten);
		if ( written < 0 ) { perror("write"); return false; }
		bytesWritten += written;
	}

	return true;
}

void usage(int /*argc*/, char* argv[])
{
	printf("usage: %s [OPTIONS] [FILE]...\n", argv[0]);
	printf("Preprocess FILE(s), or standard input.");
	printf("\n");
	printf("Options:\n");
	printf("  -o <file>                Write output to this file\n");
	printf("                           Default = Standard Output\n");
	printf("  -I <file>                Add this directory to the include search paths\n");
	printf("                           If no paths are set, include from working dir\n");
	printf("  -q                       Surpress normal output\n");
	printf("  -v                       Be verbose\n");
	printf("  --usage                  Display this screen\n");
	printf("  --help                   Display this screen\n");
	printf("  --version                Display version information\n");
}

void version()
{
	printf("The Maxsi Macro PreProcessor 0.1\n");
	printf("Copyright (C) 2011 Jonas 'Sortie' Termansen\n");
	printf("This is free software; see the source for copying conditions.  There is NO\n");
	printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
	printf("website: http://www.maxsi.org/software/mxmpp/\n");
}

struct searchpath
{
	const char* path;
	searchpath* next;
};

struct inputfile
{
	const char* path;
	inputfile* next;
};

int outfd = -1;

searchpath* firstpath = NULL;
inputfile* firstfile = NULL;

const size_t CONSTRUCT_SIZE = 511;
const size_t BUFFER_SIZE = 4096;

bool verbose = false;

char* search(const char* filename);
bool include(const char* parameter);
bool expand(const char* command, const char* parameter);
bool process(int fd);
bool process(const char* path);

char* search(const char* filename)
{
	size_t filenamelen = strlen(filename);

	for ( searchpath* path = firstpath; path != NULL; path = path->next )
	{
		size_t searchpathlen = strlen(path->path);
		size_t filepathlen = searchpathlen + 1 + filenamelen;
		char* filepath = new char[filepathlen + 1];
		strcpy(filepath, path->path);
		strcpy(filepath + searchpathlen, "/");
		strcpy(filepath + searchpathlen + 1, filename);

		struct stat sts;
		int statrs = stat(filepath, &sts);
		if ( statrs < 0 && errno != ENOENT )
		{
			fprintf(stderr, "error: could not stat file '%s': %s\n", filepath, strerror(errno)); return NULL;
		}

		bool found = ( statrs != -1 );

		if ( verbose )
		{
			fprintf(stderr, "info: searching for '%s': %s\n", filepath, ( found ) ? "found" : "not found");
		}

		if ( found )
		{
			return filepath;
		}

		delete[] filepath;
	}

	return NULL;
}

bool include(const char* parameter)
{
	if ( parameter[0] == '\0' )
	{
		fprintf(stderr, "error: @include expects a non-empty parameter\n");
		return false;
	}

	if ( parameter[0] == '/' )
	{
		return process(parameter);
	}

	char* included = search(parameter);

	if ( included == NULL )
	{
		fprintf(stderr, "error: could not find included file '%s'\n", parameter);
		return false;
	}

	bool result = process(included);
	delete[] included;

	return result;
}

bool expand(const char* command, const char* parameter)
{
	if ( strcmp(command, "@include") == 0 )
	{
		return include(parameter);
	}
	else
	{
		fprintf(stderr, "error: unknown macro command %s\n", command);
		return false;
	}
}

bool process(int fd)
{
	char construct[CONSTRUCT_SIZE+1];
	char buffer[BUFFER_SIZE];
	size_t constructed = 0;
	int phase = 0;

	const char* command = NULL;
	const char* parameter = NULL;

	bool quote = false;
	bool backslash = false;

	while ( true )
	{
		ssize_t numread = read(fd, buffer, BUFFER_SIZE);
		if ( numread == 0 ) { break; }
		if ( numread < 0 ) { perror("read"); return false; }
		size_t writefrom = 0;

		for ( ssize_t i = 0; i < numread; i++ )
		{
			if ( constructed == 0 )
			{
				if ( buffer[i] == '\\' ) { backslash = !backslash; continue; }
				if ( buffer[i] == '"' && !backslash ) { quote = !quote; }
				if ( buffer[i] == '@' && !quote )
				{
					if ( i - writefrom > 0 )
					{
						if ( !writeall(outfd, buffer + writefrom, i - writefrom) ) { return false; }
					}
					construct[0] = '@'; constructed++;
				}
				backslash = false;
			}
			else
			{
				if ( phase == 0 )
				{
					if ( ( ('a' <= buffer[i] && buffer[i] <= 'z' ) ||
					       ('A' <= buffer[i] && buffer[i] <= 'Z' ) ) &&
						 ( constructed < CONSTRUCT_SIZE) )
					{
						construct[constructed] = buffer[i]; constructed++;
					}
					else if ( buffer[i] == '(' )
					{
						construct[constructed] = '\0'; constructed++;
						command = construct;
						parameter = construct + constructed;
						phase++;
						continue;
					}
					else
					{
						construct[constructed] = '\0';
						fprintf(stderr, "error: expected '(' after '%s'\n", construct);
						return false;
					}
				}
				if ( phase == 1 )
				{
					if ( buffer[i] == ')' )
					{
						construct[constructed] = '\0';
						if ( !expand(command, parameter) ) { return false; }
						phase = 0;
						constructed = 0;
						writefrom = i + 1;
					}
					else if ( buffer[i] != '\n' && buffer[i] != '\r' && constructed < CONSTRUCT_SIZE )
					{
						construct[constructed] = buffer[i]; constructed++;
					}
					else
					{
						construct[constructed] = '\0';
						fprintf(stderr, "error: expected ')' after '%s'\n", parameter);
						return false;
					}
				}
			}
		}

		if ( constructed == 0 && numread - writefrom > 0 )
		{
			if ( !writeall(outfd, buffer + writefrom, numread - writefrom) ) { return false; }
		}
	}

	return true;
}

bool process(const char* path)
{
	int fd;
	if ( strcmp(path, "-") == 0 ) { fd = 0; }
	else if ( (fd = open(path, O_RDONLY) ) < 0 )
	{
		fprintf(stderr, "error: couldn't open file '%s'\n", path); return false;
	}

	if ( verbose )
	{
		fprintf(stderr, "info: including file '%s'\n", path);
	}

	bool result = process(fd);

	if ( verbose )
	{
		fprintf(stderr, "info: end of file '%s'\n", path);
	}

	if ( close(fd) < 0 )
	{
		perror("close"); return false;
	}

	return result;
}

int main(int argc, char* argv[])
{
	const char* dest = NULL;
	searchpath* lastpath = NULL;
	inputfile* lastfile = NULL;

	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp(argv[i], "-I") == 0 )
		{
			if ( 2 <= argc - i )
			{
				searchpath* path = new searchpath();
				path->path = argv[i+1];
				path->next = NULL;
				if ( firstpath == NULL ) { firstpath = path; } else { lastpath->next = path; }
				lastpath = path;
				i++;
			}
		}
		else if ( strcmp(argv[i], "-o") == 0 )
		{
			if ( 2 <= argc - i )
			{
				dest = argv[i+1];
				i++;
			}
		}
		else if ( strcmp(argv[i], "-v") == 0 )
		{
			verbose = true;
		}
		else if ( strcmp(argv[i], "-q") == 0 )
		{
			verbose = false;
		}
		else if ( strcmp(argv[i], "--help") == 0 )
		{
			usage(argc, argv);
			return 0;
		}
		else if ( strcmp(argv[i], "--usage") == 0 )
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
			inputfile* file = new inputfile();
			file->path = argv[i];
			file->next = NULL;
			if ( firstfile == NULL ) { firstfile = file; } else { lastfile->next = file; }
			lastfile = file;
		}
	}

	if ( dest == NULL || strcmp(dest, "-") == 0 ) { outfd = 1; }
	else if ( (outfd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) ) < 0 )
	{
		fprintf(stderr, "error: couldn't open file '%s'\n", dest); return 1;
	}

	if ( firstpath == NULL )
	{
		firstpath = new searchpath;
		firstpath->path = ".";
		firstpath->next = NULL;
		lastpath = firstpath;
	}

	if ( firstfile == NULL )
	{
		firstfile = new inputfile;
		firstfile->path = "-";
		firstfile->next = NULL;
		lastfile = firstfile;
	}

	inputfile* tmp = firstfile;
	while ( tmp != NULL )
	{
		if ( !process(tmp->path) ) { return 1; }
		tmp = tmp->next;
	}

	if ( close(outfd) < 0 )
	{
		perror("close"); return 1;
	}

	return 0;
}
