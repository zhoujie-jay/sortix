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

	stdio.h
	Standard buffered input/output.

******************************************************************************/

#ifndef	_STDIO_H
#define	_STDIO_H 1

#include <features.h>

__BEGIN_DECLS

@include(off_t.h)
@include(size_t.h)
@include(ssize_t.h)
@include(va_list.h)
@include(NULL.h)

@include(FILE.h)

struct _fpos_t;
typedef struct _fpos_t fpos_t;

/* TODO: Implement L_ctermid */
#if __POSIX_OBSOLETE <= 200801
/* TODO: Implement L_tmpnam */
#endif

/* The possibilities for the third argument to `fseek'.
   These values should not be changed.  */
@include(SEEK_SET.h)
@include(SEEK_CUR.h)
@include(SEEK_END.h)

/* The possibilities for the third argument to `setvbuf'. */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

#define EOF (-1)

/* FILENAME_MAX, FOPEN_MAX, TMP_MAX are not defined because libmaxsi and Sortix
   do not have these restrictions. */

/* Default path prefix for `tempnam' and `tmpnam'. */
#if __POSIX_OBSOLETE <= 200801
#define P_tmpdir "/tmp"
#endif

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

/* C89/C99 say they're macros. Make them happy. */
#define stdin stdin
#define stdout stdout
#define stderr stderr

extern void clearerr(FILE* stream);
extern int fclose(FILE* stream);
extern int feof(FILE* stream);
extern int ferror(FILE* stream);
extern int fflush(FILE* stream);
extern int fileno(FILE* stream);
extern int fprintf(FILE* restrict stream, const char* restrict format, ...);
extern size_t fread(void* restrict ptr, size_t size, size_t nitems, FILE* restrict stream);
extern int fseek(FILE* stream, long offset, int whence);
extern long ftell(FILE* stream);
extern size_t fwrite(const void* restrict ptr, size_t size, size_t nitems, FILE* restrict stream);
extern void perror(const char* s);
extern int printf(const char* restrict format, ...);
extern void rewind(FILE* stream);
extern int vfprintf(FILE* restrict stream, const char* restrict format, va_list ap);
extern int vprintf(const char* restrict format, va_list ap);

/* TODO: These are not implemented in libmaxsi/sortix yet. */
#ifndef SORTIX_UNIMPLEMENTED
extern char* ctermid(char* s);
extern char* fgets(char* restrict s, int n, FILE* restrict stream);
extern FILE* fdopen(int fildes, const char* mode);
extern FILE *fmemopen(void* restrict buf, size_t size, const char* restrict mode);
extern FILE* fopen(const char* restrict filename, const char* restrict mode);
extern FILE* freopen(const char* restrict filename, const char *restrict mode, FILE* restrict stream);
extern FILE* open_memstream(char** bufp, size_t* sizep);
extern FILE* popen(const char* command, const char* mode);
extern FILE* tmpfile(void);
extern int dprintf(int fildes, const char* restrict format, ...);
extern int fgetc(FILE* stream);
extern int fgetpos(FILE* restrict stream, fpos_t* restrict pos);
extern int fputc(int c, FILE* stream);
extern int fputs(const char* restrict s, FILE* restrict stream);
extern int fscanf(FILE* restrict stream, const char* restrict format, ... );
extern int fseeko(FILE* stream, off_t offset, int whence);
extern int fsetpos(FILE* stream, const fpos_t* pos);
extern int ftrylockfile(FILE* file);
extern int getc(FILE* stream);
extern int getchar_unlocked(void);
extern int getchar(void);
extern int getc_unlocked(FILE* stream);
extern int pclose(FILE* steam);
extern int putchar(int c);
extern int putchar_unlocked(int c);
extern int putc(int c, FILE* stream);
extern int putc_unlocked(int c, FILE* steam);
extern int puts(const char* s);
extern int remove(const char* path);
extern int rename(const char* oldname, const char* newname);
extern int renameat(int oldfd, const char* oldname, int newfd, const char* newname);
extern int scanf(const char* restrict format, ...);
extern int setvbuf(FILE* restrict stream, char* restrict buf, int type, size_t size);
extern int snprintf(char* restrict s, size_t n, const char* restrict format, ...);
extern int sprintf(char* restrict s, const char* restrict format, ...);
extern int sscanf(const char* restrict s, const char* restrict format, ...);
extern int ungetc(int c, FILE* stream);
extern int vdprintf(int fildes, const char* restrict format, va_list ap);
extern int vfscanf(FILE* restrict stream, const char* restrict format, va_list arg);
extern int vscanf(const char* restrict format, va_list arg);
extern int vsnprintf(char* restrict, size_t, const char* restrict, va_list);
extern int vsprintf(char* restrict s, const char* restrict format, va_list ap);
extern int vsscanf(const char* restrict s, const char* restrict format, va_list arg);
extern off_t ftello(FILE* stream);
extern ssize_t getdelim(char** restrict lineptr, size_t* restrict n, int delimiter, FILE* restrict stream);
extern ssize_t getline(char** restrict lineptr, size_t* restrict n, FILE* restrict stream);
extern void flockfile(FILE* file);
extern void funlockfile(FILE* file);
extern void setbuf(FILE* restrict stream, char* restrict buf);

#if __POSIX_OBSOLETE <= 200801
extern char* gets(char* s);
extern char* tmpnam(char* s);
extern char* tempnam(const char* dir, const char* pfx);
#endif
#endif

#ifdef SORTIX_EXTENSIONS
void fregister(FILE* fp);
void funregister(FILE* fp);
FILE* fnewfile(void);
int fcloseall(void);
#endif

__END_DECLS

#endif
