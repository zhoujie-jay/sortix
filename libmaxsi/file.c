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
	FILE* in libmaxsi is an interface to various implementations of the FILE*
	API. This allows stuff like fmemopen, but also allows the application
	programmers to provide their own backends.

******************************************************************************/

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE* firstfile = NULL;

void fregister(FILE* fp)
{
	fp->flags |= _FILE_REGISTERED;
	if ( !firstfile ) { firstfile = fp; return; }
	fp->next = firstfile;
	firstfile->prev = fp;
	firstfile = fp;
}

void funregister(FILE* fp)
{
	if ( !(fp->flags & _FILE_REGISTERED) ) { return; }
	if ( !fp->prev ) { firstfile = fp->next; }
	if ( fp->prev ) { fp->prev->next = fp->next; }
	if ( fp->next ) { fp->next->prev = fp->prev; }
	fp->flags &= ~_FILE_REGISTERED;
}

size_t fread(void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( !fp->read_func ) { errno = EBADF; return 0; }
	fp->flags &= ~_FILE_LAST_WRITE; fp->flags |= _FILE_LAST_READ;
	return fp->read_func(ptr, size, nmemb, fp->user);
}

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* fp)
{
	if ( !fp->write_func ) { errno = EBADF; return 0; }
	fp->flags &= ~_FILE_LAST_READ; fp->flags |= _FILE_LAST_WRITE;
	char* str = (char*) ptr;
	size_t total = size * nmemb;
	size_t sofar = 0;
	while ( sofar < total )
	{
		size_t left = total - sofar;
		if ( (!fp->bufferused && fp->buffersize <= left) || (fp->flags & _FILE_NO_BUFFER) )
		{
			return sofar + fp->write_func(str + sofar, 1, left, fp->user);
		}

		size_t available = fp->buffersize - fp->bufferused;
		size_t count = ( left < available ) ? left : available;
		count = left;
		for ( size_t i = 0; i < count; i++ )
		{
			char c = str[sofar++];
			fp->buffer[fp->bufferused++] = c;
			if ( c == '\n' )
			{
				if ( fflush(fp) ) { return sofar; }
				break;
			}
		}

		if ( fp->buffersize <= fp->bufferused )
		{
			if ( fflush(fp) ) { return sofar; }
		}
	}
	return sofar;
}

int fseeko(FILE* fp, off_t offset, int whence)
{
	return (fp->seek_func) ? fp->seek_func(fp->user, offset, whence) : 0;
}

int fseek(FILE* fp, long offset, int whence)
{
	return fseeko(fp, offset, whence);
}

void clearerr(FILE* fp)
{
	if ( fp->clearerr_func ) { fp->clearerr_func(fp->user); }
}

int ferror(FILE* fp)
{
	if ( !fp->error_func ) { return 0; }
	return fp->error_func(fp->user);
}

int feof(FILE* fp)
{
	if ( !fp->eof_func ) { return 0; }
	return fp->eof_func(fp->user);
}

void rewind(FILE* fp)
{
	fseek(fp, 0L, SEEK_SET);
	clearerr(fp);
}

off_t ftello(FILE* fp)
{
	if ( !fp->tell_func ) { errno = EBADF; return -1; }
	return fp->tell_func(fp->user);
}

long ftell(FILE* fp)
{
	return (long) ftello(fp);
}

int fflush(FILE* fp)
{
	if ( !fp )
	{
		int result = 0;
		for ( fp = firstfile; fp; fp = fp->next ) { result |= fflush(fp); }
		return result;
	}
	if ( !fp->write_func ) { errno = EBADF; return EOF; }
	if ( !fp->bufferused ) { return 0; }
	size_t written = fp->write_func(fp->buffer, 1, fp->bufferused, fp->user);
	if ( written < fp->bufferused ) { return EOF; }
	fp->bufferused = 0;
	return 0;
}

int fclose(FILE* fp)
{
	int result = fflush(fp);
	result |= (fp->close_func) ? fp->close_func(fp->user) : 0;
	funregister(fp);
	if ( fp->free_func ) { fp->free_func(fp); }
	return result;
}

int fileno(FILE* fp)
{
	int result = (fp->fileno_func) ? fp->fileno_func(fp->user) : -1;
	if ( result < 0 ) { errno = EBADF; }
	return result;
}

size_t fbufsize(FILE* fp)
{
	return fp->buffersize;
}

int freading(FILE* fp)
{
	if ( fp->read_func ) { return 1; }
	if ( fp->flags & _FILE_LAST_READ ) { return 1; }
	return 0;
}

int fwriting(FILE* fp)
{
	if ( fp->write_func ) { return 1; }
	if ( fp->flags & _FILE_LAST_WRITE ) { return 1; }
	return 0;
}

int freadable(FILE* fp)
{
	return fp->read_func != NULL;
}

int fwritable(FILE* fp)
{
	return fp->write_func != NULL;
}

int flbf(FILE* fp)
{
	return !(fp->flags & _FILE_NO_BUFFER);
}

void fpurge(FILE* fp)
{
	fp->bufferused = 0;
}

size_t fpending(FILE* fp)
{
	return fp->bufferused;
}

int fsetlocking(FILE* fp, int type)
{
	switch ( type )
	{
	case FSETLOCKING_INTERNAL: fp->flags |= _FILE_AUTO_LOCK;
	case FSETLOCKING_BYCALLER: fp->flags &= ~_FILE_AUTO_LOCK;
	}
	return (fp->flags & _FILE_AUTO_LOCK) ? FSETLOCKING_INTERNAL
	                                     : FSETLOCKING_BYCALLER;
}

static void ffreefile(FILE* fp)
{
	free(fp->buffer);
	free(fp);
}

FILE* fnewfile(void)
{
	FILE* fp = (FILE*) calloc(sizeof(FILE), 1);
	if ( !fp ) { return NULL; }
	fp->buffersize = BUFSIZ;
	fp->buffer = (char*) malloc(fp->buffersize);
	if ( !fp->buffer ) { free(fp); return NULL; }
	fp->flags = _FILE_AUTO_LOCK;
	fp->free_func = ffreefile;
	fregister(fp);
	return fp;
}

int fcloseall(void)
{
	int result = 0;
	while ( firstfile ) { result |= fclose(firstfile); }
	return (result) ? EOF : 0;
}

void flushlbf(void)
{
	for ( FILE* fp = firstfile; fp; fp = fp->next )
	{
		fflush(fp);
	}
}

int fgetc(FILE* fp)
{
	unsigned char c;
	if ( fread(&c, 1, sizeof(c), fp) < sizeof(c) ) { return EOF; }
	return c;
}

int fputc(int cint, FILE* fp)
{
	unsigned char c = (unsigned char) cint;
	if ( fwrite(&c, 1, sizeof(c), fp) < sizeof(c) ) { return EOF; }
	return c;
}

int getc(FILE* fp)
{
	return fgetc(fp);
}

int putc(int c, FILE* fp)
{
	return fputc(c, fp);
}

int fputs(const char* str, FILE* fp)
{
	size_t stringlen = strlen(str);
	int result = fwrite(str, 1, stringlen, fp);
	if ( result < stringlen ) { return EOF; }
	return result;
}

char* fgets(char* dest, int size, FILE* fp)
{
	if ( size <= 0 ) { errno = EINVAL; return NULL; }
	int i;
	for ( i = 0; i < size-1; i++ )
	{
		int c = getc(fp);
		if ( c == EOF )
		{
			if ( ferror(fp) ) { return NULL; }
			else { i++; break; } /* EOF */
		}
		dest[i] = c;
		if ( c == '\n' ) { i++; break; }
	}

	dest[i] = '\0';
	return dest;
}

