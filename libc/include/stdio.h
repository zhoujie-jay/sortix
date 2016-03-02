/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * stdio.h
 * Standard buffered input/output.
 */

#ifndef INCLUDE_STDIO_H
#define INCLUDE_STDIO_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/seek.h>
#if __USE_SORTIX || __USE_POSIX
/* TODO: This gets too much from <stdarg.h> and not just va_list. Unfortunately
         the <stdarg.h> header that comes with gcc has no way to just request
         the declaration of va_list. Sortix libc should ship its own stdarg.h
         header instead and have a <__/stdarg.h> header we can typedef from. */
#else
#define __need___va_list
#endif
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __USE_SORTIX || 2008 <= __USE_POSIX
#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#if __USE_SORTIX || __USE_POSIX
#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif
#endif

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#ifndef __FILE_defined
#define __FILE_defined
typedef struct __FILE FILE;
#endif

#if __USE_SORTIX || 2008 <= __USE_POSIX
typedef off_t fpos_t;
#else
typedef __off_t fpos_t;
#endif

#if __USE_SORTIX || __USE_POSIX
/* TODO: Implement L_ctermid */
/* L_tmpnam will not be implemented. */
#endif

/* The possibilities for the third argument to `setvbuf'. */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* No buffering. */

#define EOF (-1)

/* FILENAME_MAX, FOPEN_MAX are not defined because Sortix doesn't have these
   restrictions. */
#if __USE_SORTIX || __USE_POSIX
/* TMP_MAX are not defined because Sortix doesn't have these restrictions. */
#endif

#if __USE_SORTIX || __USE_POSIX
/* P_tmpdir will not be implemented. */
#endif

/* Size of <stdio.h> buffers. */
#define BUFSIZ 8192UL

/* Constants used by `fparsemode'. */
#if __USE_SORTIX
#define FILE_MODE_READ (1 << 0)
#define FILE_MODE_WRITE (1 << 1)
#define FILE_MODE_APPEND (1 << 2)
#define FILE_MODE_CREATE (1 << 3)
#define FILE_MODE_TRUNCATE (1 << 4)
#define FILE_MODE_BINARY (1 << 5)
#define FILE_MODE_EXCL (1 << 6)
#define FILE_MODE_CLOEXEC (1 << 7)
#endif

extern FILE* const stdin;
extern FILE* const stdout;
extern FILE* const stderr;

#define stdin stdin
#define stdout stdout
#define stderr stderr

/* Functions from C89. */
void clearerr(FILE* stream);
int fclose(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
int fflush(FILE* stream);
int fgetc(FILE* stream);
int fgetpos(FILE* __restrict stream, fpos_t* __restrict pos);
char* fgets(char* __restrict s, int n, FILE* __restrict stream);
FILE* fopen(const char* __restrict filename, const char* __restrict mode);
int fprintf(FILE* __restrict stream, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int fputc(int c, FILE* stream);
int fputs(const char* __restrict, FILE* __restrict stream);
size_t fread(void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
FILE* freopen(const char* __restrict filename, const char *__restrict mode, FILE* __restrict stream);
int fscanf(FILE* __restrict stream, const char* __restrict format, ... )
	__attribute__((__format__ (scanf, 2, 3)));
int fseek(FILE* stream, long offset, int whence);
int fsetpos(FILE* stream, const fpos_t* pos);
long ftell(FILE* stream);
size_t fwrite(const void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
int getc(FILE* stream);
int getchar(void);
#if __USE_C < 2011
/* gets will not be implemented. */
#endif
void perror(const char* s);
int printf(const char* __restrict format, ...)
	__attribute__((__format__ (printf, 1, 2)));
int putc(int c, FILE* stream);
int putchar(int c);
int puts(const char* str);
int remove(const char* path);
int rename(const char* oldname, const char* newname);
void rewind(FILE* stream);
void setbuf(FILE* __restrict stream, char* __restrict buf);
int setvbuf(FILE* __restrict stream, char* __restrict buf, int type, size_t size);
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("sprintf() is dangerous, use snprintf()")))
#endif
int sprintf(char* __restrict s, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int scanf(const char* __restrict format, ...)
	__attribute__((__format__ (scanf, 1, 2)));
int sscanf(const char* __restrict s, const char* __restrict format, ...)
	__attribute__((__format__ (scanf, 2, 3)));
FILE* tmpfile(void);
int ungetc(int c, FILE* stream);
int vfprintf(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vprintf(const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 1, 0)));
#if !defined(__is_sortix_libc) /* not a warning inside libc */
__attribute__((__warning__("vsprintf() is dangerous, use vsnprintf()")))
#endif
int vsprintf(char* __restrict s, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));

/* Functions from C99. */
#if __USE_SORTIX || 1999 <= __USE_C
int snprintf(char* __restrict s, size_t n, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 3, 4)));
int vfscanf(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));
int vscanf(const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 1, 0)));
int vsnprintf(char* __restrict, size_t, const char* __restrict, __gnuc_va_list)
	__attribute__((__format__ (printf, 3, 0)));
int vsscanf(const char* __restrict s, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));
#endif

/* Functions from early POSIX. */
#if __USE_SORTIX || __USE_POSIX
int fileno(FILE* stream);
void flockfile(FILE* file);
FILE* fdopen(int fildes, const char* mode);
int ftrylockfile(FILE* file);
void funlockfile(FILE* file);
int getc_unlocked(FILE* stream);
int getchar_unlocked(void);
int putc_unlocked(int c, FILE* stream);
int putchar_unlocked(int c);
/* tmpnam will not be implemented. */
#endif

#if __USE_POSIX
/* TODO: ctermid */
#endif

#if __USE_XOPEN
/* tempnam will not be implemented. */
#endif

/* Functions from less early POSIX. */
#if __USE_SORTIX || 199209L <= __USE_POSIX
int pclose(FILE* steam);
FILE* popen(const char* command, const char* mode);
#endif

/* Functions from POSIX 2001. */
#if __USE_SORTIX || 200112L <= __USE_POSIX
int fseeko(FILE* stream, off_t offset, int whence);
off_t ftello(FILE* stream);
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
int dprintf(int fildes, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
FILE* fmemopen(void* __restrict, size_t, const char* __restrict);
ssize_t getdelim(char** __restrict lineptr, size_t* __restrict n, int delimiter, FILE* __restrict stream);
FILE* open_memstream(char**, size_t*);
ssize_t getline(char** __restrict lineptr, size_t* __restrict n, FILE* __restrict stream);
int renameat(int oldfd, const char* oldname, int newfd, const char* newname);
int vdprintf(int fildes, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
int asprintf(char** __restrict, const char* __restrict, ...)
	__attribute__((__format__ (printf, 2, 3)));
void clearerr_unlocked(FILE* stream);
int feof_unlocked(FILE* stream);
int ferror_unlocked(FILE* stream);
int fflush_unlocked(FILE* stream);
int fileno_unlocked(FILE* stream);
int fgetc_unlocked(FILE* stream);
char* fgets_unlocked(char* __restrict, int, FILE* __restrict);
int fputc_unlocked(int c, FILE* stream);
int fputs_unlocked(const char* __restrict, FILE* __restrict stream);
size_t fread_unlocked(void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
size_t fwrite_unlocked(const void* __restrict ptr, size_t size, size_t nitems, FILE* __restrict stream);
int vasprintf(char** __restrict, const char* __restrict, __gnuc_va_list)
	__attribute__((__format__ (printf, 2, 0)));
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
int fparsemode(const char*);
int fpipe(FILE* pipes[2]);
int fprintf_unlocked(FILE* __restrict stream, const char* __restrict format, ...)
	__attribute__((__format__ (printf, 2, 3)));
int fscanf_unlocked(FILE* __restrict stream, const char* __restrict format, ... )
	__attribute__((__format__ (scanf, 2, 3)));
int fseeko_unlocked(FILE* stream, off_t offset, int whence);
off_t ftello_unlocked(FILE* stream);
int removeat(int dirrfd, const char* path);
int setvbuf_unlocked(FILE* __restrict stream, char* __restrict buf, int type, size_t size);
char* sortix_gets(void);
int sortix_puts(const char* str);
int ungetc_unlocked(int c, FILE* stream);
int vfprintf_unlocked(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 2, 0)));
int vfscanf_unlocked(FILE* __restrict stream, const char* __restrict format, __gnuc_va_list arg)
	__attribute__((__format__ (scanf, 2, 0)));
#endif

/* Functions that are Sortix extensions used for libc internal purposes. */
#if __USE_SORTIX
int fflush_stop_reading(FILE* fp);
int fflush_stop_reading_unlocked(FILE* fp);
int fflush_stop_writing(FILE* fp);
int fflush_stop_writing_unlocked(FILE* fp);
void fdeletefile(FILE* fp);
void fregister(FILE* fp);
void fresetfile(FILE* fp);
void funregister(FILE* fp);
FILE* fnewfile(void);
int fshutdown(FILE* fp);
#endif

/* The backends for printf and scanf. */
#if __USE_SORTIX
int cbprintf(void*, size_t (*)(void*, const char*, size_t), const char*, ...)
	__attribute__((__format__ (printf, 3, 4)));
int vcbprintf(void*, size_t (*)(void*, const char*, size_t), const char*, __gnuc_va_list ap)
	__attribute__((__format__ (printf, 3, 0)));
int vscanf_callback(void* fp,
                    int (*fgetc)(void*),
                    int (*ungetc)(int, void*),
                    const char* __restrict format,
                    __gnuc_va_list ap)
	__attribute__((__format__ (scanf, 4, 0)));
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__is_sortix_libc)
#include <FILE.h>
#endif

#endif
