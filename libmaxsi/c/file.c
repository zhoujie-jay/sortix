/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	file.c
	Implements every related to the FILE structure. This API is not compatible
	enough with LibMaxsi's design goals, so it is implemented as a layer upon
	the C functions in LibMaxsi.

******************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <fcntl.h>

#if 0

// TODO: Make a real errno system!
volatile int errno;
#define EINVAL 1

typedef struct
{
  off_t pos;
  mbstate_t state;
} _fpos_t;

struct _IO_FILE
{
	int fd;
	_fpos_t pos;
};

// TODO: Actually implement these stubs.

char* fgets(char* restrict s, int n, FILE* restrict stream)
{
	return NULL;
}

FILE* fdopen(int fildes, const char* mode)
{
	return NULL;
}

FILE *fmemopen(void* restrict buf, size_t size, const char* restrict mode)
{
	return NULL;
}

FILE* fopen(const char* restrict filename, const char* restrict mode)
{
	int len;
	if ( mode[0] == '\0' ) { errno = EINVAL; return NULL; } else
	if ( mode[1] == '\1' ) { len = 1; } else
	if ( mode[2] == '\0' ) { len = 2; }
	if ( mode[3] == '\0' ) { len = 3; } else { errno = EINVAL; return NULL; }

	int oflags;

	if ( len == 1 || (len == 2 && mode[1] == 'b') )
	{
		switch ( mode[0] )
		{
			case 'r': oflags = O_RDONLY; break;
			case 'w': oflags = O_WRONLY | O_TRUNC | O_CREAT; break;
			case 'a': oflags = O_WRONLY | O_APPEND | O_CREAT; break;
			default: errno = EINVAL; return NULL;
		}
	}
	else if ( (len == 2 && mode[1] == '+') || (len == 3 && mode[1] == '+' && mode[1] == 'b') || (len == 3 && mode[1] == 'b' && mode[1] == '+') )
	{
		switch ( mode[0] )
		{
			case 'r': oflags = O_RDWR; break;
			case 'w': oflags = O_RDWR | O_TRUNC | O_CREAT; break;
			case 'a': oflags = O_RDWR | O_APPEND | O_CREAT; break;
			default: errno = EINVAL; return NULL;
		}
	}
	else { errno = EINVAL; return NULL; }

	// TODO: Does anything else modify this mask?
	// TODO: POSIX says this should be "S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH",
	// but Linux applies this in a simple test case!
	mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	FILE* file = malloc(sizeof(FILE));

	if ( file == NULL ) { return NULL; }

	int fd = open(filename, oflags, perms);

	if ( fd < 0 ) { free(file); return NULL; }

	file->fd = fd;
	// TODO: set other stuff here!

	return file;
}

FILE* freopen(const char* restrict filename, const char *restrict mode, FILE* restrict stream)
{
	return NULL;
}

FILE* popen(const char* command, const char* mode)
{
	return NULL;
}

FILE* tmpfile(void)
{
	return NULL;
}

int fclose(FILE* stream)
{
	return -1;
}

int feof(FILE* stream)
{
	return -1;	
}

int ferror(FILE* stream)
{
	return -1;
}

int fflush(FILE* stream)
{
	return -1;
}

int fgetc(FILE* stream)
{
	return -1;
}

int fgetpos(FILE* restrict stream, fpos_t* restrict pos)
{
	return -1;
}

int fileno(FILE* stream)
{
	return -1;
}

int fprintf(FILE* restrict stream, const char* restrict format, ...)
{
	return -1;
}

int fputc(int c, FILE* stream)
{
	return -1;
}

int fputs(const char* restrict s, FILE* restrict stream)
{
	return -1;
}

int fscanf(FILE* restrict stream, const char* restrict format, ... )
{
	return -1;
}

int fseek(FILE* stream, long offset, int whence)
{
	return -1;
}

int fseeko(FILE* stream, off_t offset, int whence)
{
	return -1;
}

int fsetpos(FILE* stream, const fpos_t* pos)
{
	return -1;
}

int ftrylockfile(FILE* file)
{
	return -1;
}

int getc(FILE* stream)
{
	return -1;
}

int getc_unlocked(FILE* stream)
{
	return -1;
}

int pclose(FILE* steam)
{
	return -1;
}

int putc(int c, FILE* stream)
{
	return -1;
}

int putc_unlocked(int c, FILE* steam)
{
	return -1;
}

int setvbuf(FILE* restrict stream, char* restrict buf, int type, size_t size)
{
	return -1;
}

int ungetc(int c, FILE* stream)
{
	return -1;
}

int vfprintf(FILE* restrict stream, const char* restrict format, va_list ap)
{
	return -1;
}

int vfscanf(FILE* restrict stream, const char* restrict format, va_list arg)
{
	return -1;
}

int vprintf(FILE* restrict stream, const char* restrict format, va_list ap)
{
	return -1;
}

long ftell(FILE* stream)
{
	return -1;
}

off_t ftello(FILE* stream)
{
	return -1;
}

size_t fread(void* restrict ptr, size_t size, size_t nitems, FILE* restrict stream)
{
	return 0;
}

size_t fwrite(const void* restrict ptr, size_t size, size_t nitems, FILE* restrict stream)
{
	return 0;
}

void clearerr(FILE* stream)
{
		
}

void flockfile(FILE* file)
{
	
}

void funlockfile(FILE* file)
{
	
}

void rewind(FILE* stream)
{
	
}

void setbuf(FILE* restrict stream, char* restrict buf)
{
	
}

#endif

