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
#include <dirent.h>
#include <unistd.h>

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

