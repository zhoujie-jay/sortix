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

	signal.cpp
	Handles the good old unix signals.

******************************************************************************/

#include "platform.h"
#include "string.h"
#include "memory.h"
#include "syscall.h"
#include "process.h"
#include "signal.h"
#include <signal.h>

namespace Maxsi
{
	namespace Signal
	{
		void Core(int signum)
		{
			 Process::Exit(128 + signum);
		}

		extern "C" void SIG_DFL(int signum)
		{
			if ( signum == Signal::HUP ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::INT ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::QUIT ) { Core(signum); } else
			if ( signum == Signal::TRAP ) { Core(signum); } else
			if ( signum == Signal::ABRT ) { Core(signum); } else
			if ( signum == Signal::EMT ) { Core(signum); } else
			if ( signum == Signal::FPE ) { Core(signum); } else
			if ( signum == Signal::KILL ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::BUS ) { Core(signum); } else
			if ( signum == Signal::SEGV ) { Core(signum); } else
			if ( signum == Signal::SYS ) { Core(signum); } else
			if ( signum == Signal::PIPE ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::ALRM ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::TERM ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::USR1 ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::USR2 ) { Process::Exit(128 + signum); } else
			if ( signum == Signal::CHLD ) { /* Ignore this signal. */ } else
			if ( signum == Signal::PWR ) { /* Ignore this signal. */ } else
			if ( signum == Signal::WINCH ) { /* Ignore this signal. */ } else
			if ( signum == Signal::URG ) { /* Ignore this signal. */ } else
			if ( signum == Signal::CONT ) { /* Ignore this signal. */ } else
			if ( signum == Signal::VTALRM ) { /* Ignore this signal. */ } else
			if ( signum == Signal::XCPU ) { Core(signum); } else
			if ( signum == Signal::XFSZ ) { Core(signum); } else
			if ( signum == Signal::WAITING ) { /* Ignore this signal. */ } else
			if ( signum == Signal::LWP ) { /* Ignore this signal. */ } else
			if ( signum == Signal::AIO ) { /* Ignore this signal. */ } else
			{ /* Ignore this signal. */ }
		}

		extern "C" void SIG_IGN(int /*signum*/)
		{
			
		}

		extern "C" void SIG_ERR(int /*signum*/)
		{
			Process::Abort();
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

		DEFN_SYSCALL1_VOID(SysRegisterSignalHandler, 29, sighandler_t);
		DEFN_SYSCALL0_VOID(SysSigReturn, 30);
		DEFN_SYSCALL2(int, SysKill, 31, pid_t, int);

		void Init()
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
	}
}
