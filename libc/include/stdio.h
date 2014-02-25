/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    stdio.h
    Standard buffered input/output.

*******************************************************************************/

#ifndef INCLUDE_STDIO_H
#define INCLUDE_STDIO_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/seek.h>
#if __STRICT_ANSI__
#define __need___va_list
#endif
#include <stdarg.h>

#if defined(__is_sortix_libc)
#include <FILE.h>
#endif

__BEGIN_DECLS

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#ifndef __FILE_defined
#define __FILE_defined
typedef struct FILE FILE;
#endif

typedef off_t fpos_t;

/* TODO: Implement L_ctermid */
#define L_tmpnam 128

/* The possibilities for the third argument to `setvbuf'. */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

#define EOF (-1)

/* FILENAME_MAX, FOPEN_MAX, TMP_MAX are not defined because libc and Sortix
   do not have these restrictions. */

/* Default path prefix for `tempnam' and `tmpnam'. */
#define P_tmpdir "/tmp"

/* Size of <stdio.h> buffers. */
#define BUFSIZ 8192UL

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

/* C89/C99 say they're macros. Make them happy. */
#define stdin stdin
#define stdout stdout
#define stderr stderr

int asprintf(char** __restrict, const char* __restrict, ...)
	__attribute__((__format__ (printf, 2, 3)));
void clearerr(FILE* stream);
void clearerr_unlocked(FILE* stream);
int dprintf(int fildes, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int fclose(FILE* stream);
FILE* fdopen(int fildes, const char* mode);
int feof(FILE* stream);
int feof_unlocked(FILE* stream);
int ferror(FILE* stream);
int ferror_unlocked(FILE* stream);
int fflush(FILE* stream);
int fflush_unlocked(FILE* stream);
int fileno(FILE* stream);
int fileno_unlocked(FILE* stream);
int fgetc(FILE* stream);
int fgetc_unlocked(FILE* stream);
int fgetpos(FILE* __restrict stream, fpos_t* __restrict pos);
char* fgets(char* __restrict s, int n, FILE* __restrict stream);
char* fgets_unlocked(char* __restrict, int, FILE* __restrict);
void flockfile(FILE* file);
FILE* fmemopen(void* __restrict, size_t, const char* __restrict);
FILE* fopen(const char* __restrict filename, const char* __restrict mode);
int fprintf(FILE* __restrict stream, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int fprintf_unlocked(FILE* __restrict stream, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int fputc(int c, FILE* stream);
int fputc_unlocked(int c, FILE* stream);
int fputs(const char* __restrict, FILE* __restrict stream);
int fputs_unlocked(const char* __restrict, FILE* __restrict stream);
size_t fread(void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
size_t fread_unlocked(void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
FILE* freopen(const char* __restrict filename, const char *__restrict mode, FILE* __restrict stream);
int fscanf(FILE* __restrict stream, const char* __restrict format, ... )
	__attribute__((__format__ (scanf, 2, 3)));
int fscanf_unlocked(FILE* __restrict stream, const char* __restrict format, ... )
	__attribute__((__format__ (scanf, 2, 3)));
int fseek(FILE* stream, long offset, int whence);
int fseeko(FILE* stream, off_t offset, int whence);
int fseeko_unlocked(FILE* stream, off_t offset, int whence);
int fsetpos(FILE* stream, const fpos_t* pos);
long ftell(FILE* stream);
off_t ftello(FILE* stream);
off_t ftello_unlocked(FILE* stream);
int ftrylockfile(FILE* file);
void funlockfile(FILE* file);
size_t fwrite(const void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
size_t fwrite_unlocked(const void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
int getc(FILE* stream);
int getc_unlocked(FILE* stream);
int getchar(void);
int getchar_unlocked(void);
ssize_t getdelim(char** __restrict lineptr, size_t* __restrict n, int delimiter, FILE* __restrict stream);
ssize_t getline(char** __restrict lineptr, size_t* __restrict n, FILE* __restrict stream);
int pclose(FILE* steam);
void perror(const char* s);
FILE* popen(const char* command, const char* mode);
int printf(const char* __restrict format, ...)
	__attribute__((__format__ (printf, 1, 2)));
int putc(int c, FILE* stream);
int putc_unlocked(int c, FILE* stream);
int putchar(int c);
int putchar_unlocked(int c);
int puts(const char* str);
int removeat(int dirrfd, const char* path);
int remove(const char* path);
int renameat(int oldfd, const char* oldname, int newfd, const char* newname);
int rename(const char* oldname, const char* newname);
void rewind(FILE* stream);
int snprintf(char* __restrict s, size_t n, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 3, 4)));
void setbuf(FILE* __restrict stream, char* __restrict buf);
int setvbuf(FILE* __restrict stream, char* __restrict buf, int type, size_t size);
int setvbuf_unlocked(FILE* __restrict stream, char* __restrict buf, int type, size_t size);
char* sortix_gets(void);
int sortix_puts(const char* str);
int sprintf(char* __restrict s, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int scanf(const char* __restrict format, ...)
	__attribute__((__format__ (scanf, 1, 2)));
int sscanf(const char* __restrict s, const char* __restrict format, ...)
	__attribute__((__format__ (scanf, 2, 3)));
FILE* tmpfile(void);
char* tmpnam(char* s);
int ungetc(int c, FILE* stream);
int ungetc_unlocked(int c, FILE* stream);
int vasprintf(char** __restrict, const char* __restrict, __gnuc_va_list)
	__attribute__((__format__ (printf, 2, 0)));
int vdprintf(int fildes, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vfprintf(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vfprintf_unlocked(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vfscanf(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));
int vfscanf_unlocked(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));
int vprintf(const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 1, 0)));
int vscanf(const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 1, 0)));
int vsnprintf(char* __restrict, size_t, const char* __restrict, __gnuc_va_list)
	__attribute__((__format__ (printf, 3, 0)));
int vsprintf(char* __restrict s, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vsscanf(const char* __restrict s, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));

/* TODO: These are not implemented in sortix libc yet. */
#if 0
char* ctermid(char* s);
FILE* open_memstream(char** bufp, size_t* sizep);
#endif

#if defined(_SORTIX_SOURCE)

#define FILE_MODE_READ (1 << 0)
#define FILE_MODE_WRITE (1 << 1)
#define FILE_MODE_APPEND (1 << 2)
#define FILE_MODE_CREATE (1 << 3)
#define FILE_MODE_TRUNCATE (1 << 4)
#define FILE_MODE_BINARY (1 << 5)
#define FILE_MODE_EXCL (1 << 6)
#define FILE_MODE_CLOEXEC (1 << 7)

#include <stdio_ext.h>
#define fbufsize __fbufsize
size_t fbufsize_unlocked(FILE* fp);
#define freading __freading
int freading_unlocked(FILE* fp);
#define fwriting __fwriting
int fwriting_unlocked(FILE* fp);
#define freadable __freadable
int freadable_unlocked(FILE* fp);
#define fwritable __fwritable
int fwritable_unlocked(FILE* fp);
#define flbf __flbf
int flbf_unlocked(FILE* fp);
#define fpurge __fpurge
void fpurge_unlocked(FILE* fp);
#define fpending __fpending
size_t fpending_unlocked(FILE* fp);
#define flushlbf _flushlbf
#define fsetlocking __fsetlocking
int fsetlocking_unlocked(FILE* fp, int type);
int fflush_stop_reading(FILE* fp);
int fflush_stop_reading_unlocked(FILE* fp);
int fflush_stop_writing(FILE* fp);
int fflush_stop_writing_unlocked(FILE* fp);
void fdeletefile(FILE* fp);
void fseterr(FILE* fp);
void fseterr_unlocked(FILE* fp);
void fregister(FILE* fp);
void fresetfile(FILE* fp);
void funregister(FILE* fp);
FILE* fnewfile(void);
int fsetdefaultbuf(FILE* fp);
int fsetdefaultbuf_unlocked(FILE* fp);
int fcloseall(void);
int fshutdown(FILE* fp);
int fpipe(FILE* pipes[2]);
int fparsemode(const char*);
#endif

#if defined(_SORTIX_SOURCE) || defined(_WANT_SORTIX_VPRINTF_CALLBACK)
size_t vprintf_callback(size_t (*callback)(void*, const char*, size_t),
                        void* user,
                        const char* __restrict format,
                        __gnuc_va_list ap)
	__attribute__((__format__ (printf, 3, 0)));
int vscanf_callback(void* fp,
                    int (*fgetc)(void*),
                    int (*ungetc)(int, void*),
                    const char* __restrict format,
                    __gnuc_va_list ap)
	__attribute__((__format__ (scanf, 4, 0)));
#endif

__END_DECLS

#endif
