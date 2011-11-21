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
#include <sys/readdirents.h>

namespace Maxsi
{
	DEFN_SYSCALL1(size_t, SysPrint, 4, const char*);
	DEFN_SYSCALL3(ssize_t, SysRead, 18, int, void*, size_t);
	DEFN_SYSCALL3(ssize_t, SysWrite, 19, int, const void*, size_t);
	DEFN_SYSCALL1(int, SysPipe, 20, int*);
	DEFN_SYSCALL1(int, SysClose, 21, int);
	DEFN_SYSCALL1(int, SysDup, 22, int);
	DEFN_SYSCALL3(int, SysOpen, 23, const char*, int, mode_t);
	DEFN_SYSCALL3(int, SysReadDirEnts, 24, int, struct sortix_dirent*, size_t);
	DEFN_SYSCALL1(int, SysChDir, 25, const char*);
	DEFN_SYSCALL2(char*, SysGetCWD, 26, char*, size_t);

	size_t Print(const char* Message)
	{
		return SysPrint(Message);
	}

	size_t PrintCallback(void* user, const char* string, size_t stringlen)
	{
		return SysPrint(string);
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

	extern "C" ssize_t read(int fd, void* buf, size_t count)
	{
		return SysRead(fd, buf, count);
	}

	extern "C" ssize_t write(int fd, const void* buf, size_t count)
	{
		return SysWrite(fd, buf, count);
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
#endif

}
