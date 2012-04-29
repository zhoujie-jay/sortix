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

	process.cpp
	Exposes system calls for process creation and management.

*******************************************************************************/

#define _WANT_ENVIRON
#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <libmaxsi/process.h>
#include <stdio.h>
#include <stdarg.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

namespace Maxsi
{
	namespace Process
	{
		DEFN_SYSCALL1_VOID(SysExit, SYSCALL_EXIT, int);
		DEFN_SYSCALL3(int, SysExecVE, SYSCALL_EXEC, const char*, char* const*, char* const*);
		DEFN_SYSCALL2(pid_t, SysSForkR, SYSCALL_SFORKR, int, sforkregs_t*);
		DEFN_SYSCALL0(pid_t, SysGetPID, SYSCALL_GETPID);
		DEFN_SYSCALL0(pid_t, SysGetParentPID, SYSCALL_GETPPID);
		DEFN_SYSCALL3(pid_t, SysWait, SYSCALL_WAIT, pid_t, int*, int);

		void Abort()
		{
			// TODO: Send SIGABRT instead!
			Exit(128 + 6);
		}

		extern "C" void abort() { return Abort(); }

		extern "C" void _exit(int status)
		{
			SysExit(status);
		}

		extern "C" int execve(const char* pathname, char* const* argv,
		                      char* const* envp)
		{
			return SysExecVE(pathname, argv, envp);
		}

		extern "C" int execv(const char* pathname, char* const* argv)
		{
			return execve(pathname, argv, environ);
		}

		// Note that the only PATH variable in Sortix is the one used here.
		extern "C" int execvpe(const char* filename, char* const* argv,
		                       char* const* envp)
		{
			if ( strchr(filename, '/') )
				return execve(filename, argv, envp);
			size_t filenamelen = strlen(filename);
			const char* PATH = "/bin";
			size_t pathlen = strlen(PATH);
			char* pathname = (char*) malloc(filenamelen + 1 + pathlen + 1);
			if ( !pathname ) { return -1; }
			stpcpy(stpcpy(stpcpy(pathname, PATH), "/"), filename);
			int result = execve(pathname, argv, envp);
			free(pathname);
			return result;
		}

		extern "C" int execvp(const char* filename, char* const* argv)
		{
			return execvpe(filename, argv, environ);
		}

		extern "C" int vexecl(const char* pathname, va_list args)
		{
			va_list iter;
			va_copy(iter, args);
			size_t numargs = 0;
			while ( va_arg(iter, const char*) ) { numargs++; }
			va_end(iter);
			char** argv = (char**) malloc(sizeof(char*) * (numargs+1));
			if ( !argv ) { return -1; }
			for ( size_t i = 0; i <= numargs; i++ )
			{
				argv[i] = (char*) va_arg(args, const char*);
			}
			int result = execv(pathname, argv);
			free(argv);
			return result;
		}

		extern "C" int vexeclp(const char* filename, va_list args)
		{
			va_list iter;
			va_copy(iter, args);
			size_t numargs = 0;
			while ( va_arg(iter, const char*) ) { numargs++; }
			va_end(iter);
			char** argv = (char**) malloc(sizeof(char*) * (numargs+1));
			if ( !argv ) { return -1; }
			for ( size_t i = 0; i <= numargs; i++ )
			{
				argv[i] = (char*) va_arg(args, const char*);
			}
			int result = execvp(filename, argv);
			free(argv);
			return result;
		}

		extern "C" int vexecle(const char* pathname, va_list args)
		{
			va_list iter;
			va_copy(iter, args);
			size_t numargs = 0;
			while ( va_arg(iter, const char*) ) { numargs++; }
			va_end(iter);
			numargs--; // envp
			char** argv = (char**) malloc(sizeof(char*) * (numargs+1));
			if ( !argv ) { return -1; }
			for ( size_t i = 0; i < numargs; i++ )
			{
				argv[i] = (char*) va_arg(args, const char*);
			}
			argv[numargs] = NULL;
			char* const* envp = va_arg(args, char* const*);
			int result = execve(pathname, argv, envp);
			free(argv);
			return result;
		}

		extern "C" int execl(const char* pathname, ...)
		{
			va_list args;
			va_start(args, pathname);
			int result = vexecl(pathname, args);
			va_end(args);
			return result;
		}

		extern "C" int execlp(const char* filename, ...)
		{
			va_list args;
			va_start(args, filename);
			int result = vexeclp(filename, args);
			va_end(args);
			return result;
		}

		extern "C" int execle(const char* pathname, ...)
		{
			va_list args;
			va_start(args, pathname);
			int result = vexecle(pathname, args);
			va_end(args);
			return result;
		}

		DUAL_FUNCTION(void, exit, Exit, (int status))
		{
			dcloseall();
			fcloseall();
			_exit(status);
		}

		extern "C" pid_t sforkr(int flags, sforkregs_t* regs)
		{
			return SysSForkR(flags, regs);
		}

		extern "C" pid_t __call_sforkr_with_regs(int flags);

		extern "C" pid_t sfork(int flags)
		{
			return __call_sforkr_with_regs(flags);
		}

		DUAL_FUNCTION(pid_t, fork, Fork, ())
		{
			return sfork(SFFORK);
		}

		DUAL_FUNCTION(pid_t, getpid, GetPID, ())
		{
			return SysGetPID();
		}

		DUAL_FUNCTION(pid_t, getppid, GetParentPID, ())
		{
			return SysGetParentPID();
		}

		extern "C" pid_t waitpid(pid_t pid, int* status, int options)
		{
			return SysWait(pid, status, options);
		}

		extern "C" pid_t wait(int* status)
		{
			return waitpid(-1, status, 0);
		}
	}
}

