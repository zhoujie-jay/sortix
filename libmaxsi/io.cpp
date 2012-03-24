/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	io.cpp
	Functions for management of input and output.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <libmaxsi/io.h>
#include <libmaxsi/format.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include <sys/readdirents.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>
#include <stdio.h>

namespace Maxsi
{
	DEFN_SYSCALL1(size_t, SysPrint, SYSCALL_PRINT_STRING, const char*);
	DEFN_SYSCALL3(ssize_t, SysRead, SYSCALL_READ, int, void*, size_t);
	DEFN_SYSCALL3(ssize_t, SysWrite, SYSCALL_WRITE, int, const void*, size_t);
	DEFN_SYSCALL1(int, SysPipe, SYSCALL_PIPE, int*);
	DEFN_SYSCALL1(int, SysClose, SYSCALL_CLOSE, int);
	DEFN_SYSCALL1(int, SysDup, SYSCALL_DUP, int);
	DEFN_SYSCALL3(int, SysOpen, SYSCALL_OPEN, const char*, int, mode_t);
	DEFN_SYSCALL3(int, SysReadDirEnts, SYSCALL_READDIRENTS, int, struct sortix_dirent*, size_t);
	DEFN_SYSCALL1(int, SysChDir, SYSCALL_CHDIR, const char*);
	DEFN_SYSCALL2(char*, SysGetCWD, SYSCALL_GETCWD, char*, size_t);
	DEFN_SYSCALL1(int, SysUnlink, SYSCALL_UNLINK, const char*);
	DEFN_SYSCALL3_VOID(SysSeek, SYSCALL_SEEK, int, off_t*, int);
	DEFN_SYSCALL2(int, SysMkDir, SYSCALL_MKDIR, const char*, mode_t);
	DEFN_SYSCALL1(int, SysRmDir, SYSCALL_RMDIR, const char*);
	DEFN_SYSCALL2(int, SysTruncate, SYSCALL_TRUNCATE, const char*, off_t);
	DEFN_SYSCALL2(int, SysFTruncate, SYSCALL_FTRUNCATE, int, off_t);
	DEFN_SYSCALL2(int, SysStat, SYSCALL_STAT, const char*, struct stat*);
	DEFN_SYSCALL2(int, SysFStat, SYSCALL_FSTAT, int, struct stat*);
	DEFN_SYSCALL3(int, SysFCntl, SYSCALL_FCNTL, int, int, unsigned long);
	DEFN_SYSCALL2(int, SysAccess, SYSCALL_ACCESS, const char*, int);

	size_t Print(const char* string)
	{
		size_t stringlen = String::Length(string);
		if ( writeall(1, string, stringlen) ) { return 0; }
		return stringlen;
	}

	size_t PrintCallback(void* user, const char* string, size_t stringlen)
	{
		if ( writeall(1, string, stringlen) ) { return 0; }
		return stringlen;
	}

	size_t PrintF(const char* format, ...)
	{
		va_list list;
		va_start(list, format);
		size_t result = Maxsi::Format::Virtual(PrintCallback, NULL, format, list);
		va_end(list);
		return result;
	}

#ifdef LIBMAXSI_LIBC
	size_t FileWriteCallback(void* user, const char* string, size_t stringlen)
	{
		FILE* fp = (FILE*) user;
		return fwrite(string, 1, stringlen, fp);
	}

	extern "C" int vfprintf(FILE* fp, const char* /*restrict*/ format, va_list list)
	{
		size_t result = Maxsi::Format::Virtual(FileWriteCallback, fp, format, list);
		return (int) result;
	}

	extern "C" int fprintf(FILE* fp, const char* /*restrict*/ format, ...)
	{
		va_list list;
		va_start(list, format);
		size_t result = vfprintf(fp, format, list);
		va_end(list);
		return (int) result;
	}

	extern "C" int vprintf(const char* /*restrict*/ format, va_list list)
	{
		size_t result = vfprintf(stdout, format, list);
		return (int) result;
	}

	extern "C" int printf(const char* /*restrict*/ format, ...)
	{
		va_list list;
		va_start(list, format);
		size_t result = vprintf(format, list);
		va_end(list);
		return (int) result;
	}

	// TODO: This is an ugly hack to help build binutils.
	#warning Ugly sscanf hack to help build binutils
	extern "C" int sscanf(const char* s, const char* format, ...)
	{
		if ( strcmp(format, "%x") != 0 )
		{
			fprintf(stderr, "sscanf hack doesn't implement: '%s'\n", format);
			abort();
		}

		va_list list;
		va_start(list, format);
		unsigned* dec = va_arg(list, unsigned*);
		*dec = strtol(s, NULL, 16);
		return strlen(s);
	}

	typedef struct vsnprintf_struct
	{
		char* str;
		size_t size;
		size_t produced;
		size_t written;
	} vsnprintf_t;

	size_t StringPrintCallback(void* user, const char* string, size_t stringlen)
	{
		vsnprintf_t* info = (vsnprintf_t*) user;
		if ( info->produced < info->size )
		{
			size_t available = info->size - info->produced;
			size_t possible = (stringlen < available) ? stringlen : available;
			Memory::Copy(info->str + info->produced, string, possible);
			info->written += possible;
		}
		info->produced += stringlen;
		return stringlen;
	}

	extern "C" int vsnprintf(char* restrict str, size_t size, const char* restrict format, va_list list)
	{
		vsnprintf_t info;
		info.str = str;
		info.size = (size) ? size-1 : 0;
		info.produced = 0;
		info.written = 0;
		Maxsi::Format::Virtual(StringPrintCallback, &info, format, list);
		if ( size ) { info.str[info.written] = '\0'; }
		return (int) info.produced;
	}

	extern "C" int snprintf(char* restrict str, size_t size, const char* restrict format, ...)
	{
		va_list list;
		va_start(list, format);
		int result = vsnprintf(str, size, format, list);
		va_end(list);
		return result;
	}

	extern "C" int vsprintf(char* restrict str, const char* restrict format, va_list list)
	{
		return vsnprintf(str, SIZE_MAX, format, list);
	}

	extern "C" int sprintf(char* restrict str, const char* restrict format, ...)
	{
		va_list list;
		va_start(list, format);
		int result = vsprintf(str, format, list);
		va_end(list);
		return result;
	}

	extern "C" void error(int status, int errnum, const char *format, ...)
	{
		fprintf(stderr, "%s: ", program_invocation_name);

		va_list list;
		va_start(list, format);
		vfprintf(stderr, format, list);
		va_end(list);

		if ( errnum ) { fprintf(stderr, ": %s", strerror(errnum)); }
		fprintf(stderr, "\n");
		if ( status ) { exit(status); }
	}

	extern "C" void perror(const char* s)
	{
		error(0, errno, "%s", s);
	}

	extern "C" ssize_t read(int fd, void* buf, size_t count)
	{
retry:
		ssize_t result = SysRead(fd, buf, count);
		if ( result < 0 && errno == EAGAIN ) { goto retry; }
		return result;
	}

	extern "C" ssize_t write(int fd, const void* buf, size_t count)
	{
retry:
		ssize_t result = SysWrite(fd, buf, count);
		if ( result < 0 && errno == EAGAIN ) { goto retry; }
		return result;
	}

	extern "C" int writeall(int fd, const void* buffer, size_t len)
	{
		const char* buf = (const char*) buffer;
		while ( len )
		{
			ssize_t byteswritten = write(fd, buf, len);
			if ( byteswritten < 0 ) { return (int) byteswritten; }
			buf += byteswritten;
			len -= byteswritten;
		}

		return 0;
	}

	extern "C" ssize_t pread(int, void*, size_t, off_t)
	{
		errno = ENOSYS;
		return -1;
	}

	extern "C" ssize_t pwrite(int, const void*, size_t, off_t)
	{
		errno = ENOSYS;
		return -1;
	}

	extern "C" off_t lseek(int fd, off_t offset, int whence)
	{
		SysSeek(fd, &offset, whence);
		return offset;
	}

	extern "C" int pipe(int pipefd[2])
	{
		return SysPipe(pipefd);
	}

	extern "C" int close(int fd)
	{
		return SysClose(fd);
	}

	extern "C" int dup(int fd)
	{
		return SysDup(fd);
	}

	extern "C" int open(const char* path, int flags, mode_t mode)
	{
		return SysOpen(path, flags, mode);
	}

	extern "C" int readdirents(int fd, struct sortix_dirent* dirent, size_t size)
	{
		return SysReadDirEnts(fd, dirent, size);
	}

	extern "C" int chdir(const char* path)
	{
		return SysChDir(path);
	}

	extern "C" char* getcwd(char* buf, size_t size)
	{
		return SysGetCWD(buf, size);
	}

	extern "C" int unlink(const char* pathname)
	{
		return SysUnlink(pathname);
	}

	extern "C" int mkdir(const char* pathname, mode_t mode)
	{
		return SysMkDir(pathname, mode);
	}

	extern "C" int rmdir(const char* pathname)
	{
		return SysRmDir(pathname);
	}

	extern "C" int truncate(const char* pathname, off_t length)
	{
		return SysTruncate(pathname, length);
	}

	extern "C" int ftruncate(int fd, off_t length)
	{
		return SysFTruncate(fd, length);
	}

	extern "C" int stat(const char* path, struct stat* st)
	{
		return SysStat(path, st);
	}

	extern "C" int lstat(const char* path, struct stat* st)
	{
		return SysStat(path, st);
	}

	extern "C" int fstat(int fd, struct stat* st)
	{
		return SysFStat(fd, st);
	}

	extern "C" int fcntl(int fd, int cmd, unsigned long arg)
	{
		return SysFCntl(fd, cmd, arg);
	}

	extern "C" int access(const char* pathname, int mode)
	{
		return SysAccess(pathname, mode);
	}

	// TODO: Implement these in the kernel.
	extern "C" int chmod(const char* path, mode_t mode)
	{
		errno = ENOTSUP;
		return -1;
	}

	// TODO: Implement these in the kernel.
	extern "C" int fchmod(int fd, mode_t mode)
	{
		errno = ENOTSUP;
		return -1;
	}

	// TODO: Implement these in the kernel.
	extern "C" mode_t umask(mode_t mask)
	{
		return 0;
	}

	// TODO: This is a hacky implementation of a stupid function.
	extern "C" char* mktemp(char* templ)
	{
		size_t templlen = strlen(templ);
		for ( size_t i = templlen-6UL; i < templlen; i++ )
		{
			templ[i] = '0' + rand() % 10;
		}
		return templ;
	}
#endif

}
