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

	io.cpp
	Functions for management of input and output.

******************************************************************************/

#include "platform.h"
#include "syscall.h"
#include "io.h"
#include "format.h"
#include "string.h"
#include <sys/readdirents.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <error.h>
#include <stdlib.h>

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
	DEFN_SYSCALL1(int, SysIsATTY, SYSCALL_ISATTY, int);

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
	extern "C" int printf(const char* /*restrict*/ format, ...)
	{
		va_list list;
		va_start(list, format);
		size_t result = Maxsi::Format::Virtual(PrintCallback, NULL, format, list);
		va_end(list);
		return (int) result;
	}

	extern "C" void error(int status, int errnum, const char *format, ...)
	{
		printf("%s: ", program_invocation_name);

		va_list list;
		va_start(list, format);
		size_t result = Maxsi::Format::Virtual(PrintCallback, NULL, format, list);
		va_end(list);

		printf(": %s\n", strerror(errnum));
		if ( status ) { exit(status); }
	}

	extern "C" void perror(const char* s)
	{
		error(0, errno, "%s", s);
	}

	extern "C" ssize_t read(int fd, void* buf, size_t count)
	{
		return SysRead(fd, buf, count);
	}

	extern "C" ssize_t write(int fd, const void* buf, size_t count)
	{
		return SysWrite(fd, buf, count);
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

	extern "C" int isatty(int fd)
	{
		return SysIsATTY(fd);
	}
#endif

}
