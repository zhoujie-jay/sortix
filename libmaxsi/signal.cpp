/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	signal.cpp
	Handles the good old unix signals.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

namespace Maxsi
{
	namespace Signal
	{
		typedef void (*Handler)(int);

		void Core(int signum)
		{
			exit(128 + signum);
		}

		extern "C" void SIG_DFL(int signum)
		{
			if ( signum == SIGHUP ) { exit(128 + signum); } else
			if ( signum == SIGINT ) { exit(128 + signum); } else
			if ( signum == SIGQUIT ) { Core(signum); } else
			if ( signum == SIGTRAP ) { Core(signum); } else
			if ( signum == SIGABRT ) { Core(signum); } else
			if ( signum == SIGEMT ) { Core(signum); } else
			if ( signum == SIGFPE ) { Core(signum); } else
			if ( signum == SIGKILL ) { exit(128 + signum); } else
			if ( signum == SIGBUS ) { Core(signum); } else
			if ( signum == SIGSEGV ) { Core(signum); } else
			if ( signum == SIGSYS ) { Core(signum); } else
			if ( signum == SIGPIPE ) { exit(128 + signum); } else
			if ( signum == SIGALRM ) { exit(128 + signum); } else
			if ( signum == SIGTERM ) { exit(128 + signum); } else
			if ( signum == SIGUSR1 ) { exit(128 + signum); } else
			if ( signum == SIGUSR2 ) { exit(128 + signum); } else
			if ( signum == SIGCHLD ) { /* Ignore this signal. */ } else
			if ( signum == SIGPWR ) { /* Ignore this signal. */ } else
			if ( signum == SIGWINCH ) { /* Ignore this signal. */ } else
			if ( signum == SIGURG ) { /* Ignore this signal. */ } else
			if ( signum == SIGCONT ) { /* Ignore this signal. */ } else
			if ( signum == SIGVTALRM ) { /* Ignore this signal. */ } else
			if ( signum == SIGXCPU ) { Core(signum); } else
			if ( signum == SIGXFSZ ) { Core(signum); } else
			if ( signum == SIGWAITING ) { /* Ignore this signal. */ } else
			if ( signum == SIGLWP ) { /* Ignore this signal. */ } else
			if ( signum == SIGAIO ) { /* Ignore this signal. */ } else
			{ /* Ignore this signal. */ }
		}

		extern "C" void SIG_IGN(int /*signum*/)
		{

		}

		extern "C" void SIG_ERR(int /*signum*/)
		{
			abort();
		}

		const int MAX_SIGNALS = 128;
		sighandler_t handlers[MAX_SIGNALS];

		extern "C" void SignalHandlerAssembly(int signum);
		extern "C" void SignalHandler(int signum)
		{
			if ( 0 <= signum && signum < (int) MAX_SIGNALS )
			{
				handlers[signum](signum);
			}
		}

		DEFN_SYSCALL1_VOID(SysRegisterSignalHandler, SYSCALL_REGISTER_SIGNAL_HANDLER, sighandler_t);
		DEFN_SYSCALL2(int, SysKill, SYSCALL_KILL, pid_t, int);
		DEFN_SYSCALL1(int, SysRaise, SYSCALL_RAISE, int);

		extern "C" void init_signal()
		{
			for ( int i = 0; i < MAX_SIGNALS; i++ )
			{
				handlers[i] = SIG_DFL;
			}

			// Tell the kernel which function we want called upon signals.
			SysRegisterSignalHandler(&SignalHandlerAssembly);
		}

		Handler RegisterHandler(int signum, Handler handler)
		{
			if ( signum < 0 || MAX_SIGNALS <= signum ) { return SIG_ERR; }
			return handlers[signum] = handler;
		}

		extern "C" sighandler_t signal(int signum, sighandler_t handler)
		{
			return RegisterHandler(signum, handler);
		}

		extern "C" int kill(pid_t pid, int signum)
		{
			return SysKill(pid, signum);
		}

		extern "C" int raise(int signum)
		{
			return SysRaise(signum);
		}
	}
}
