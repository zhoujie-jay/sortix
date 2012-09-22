/******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	signal.cpp
	Classes and functions making it easier to handle Unix signals.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/panic.h>
#include <sortix/signal.h>
#include <assert.h>
#include <string.h>
#include "interrupt.h"
#include "thread.h"
#include "signal.h"

namespace Sortix {

// A per-cpu value whether a signal is pending in the running task.
extern "C" { volatile unsigned long asm_signal_is_pending = 0; }

namespace Signal {

const int PRIORITIES[SIG__NUM_DECLARED] =
{
	SIG_PRIO_NORMAL, // unused
	SIG_PRIO_NORMAL, // SIGHUP
	SIG_PRIO_NORMAL, // SIGINT
	SIG_PRIO_NORMAL, // SIGQUIT
	SIG_PRIO_CORE, // SIGILL
	SIG_PRIO_CORE, // SIGTRAP
	SIG_PRIO_CORE, // SIGABRT
	SIG_PRIO_CORE, // SIGEMT
	SIG_PRIO_CORE, // SIGFPE
	SIG_PRIO_KILL, // SIGKILL
	SIG_PRIO_CORE, // SIGBUS
	SIG_PRIO_CORE, // SIGSEGV
	SIG_PRIO_CORE, // SIGSYS
	SIG_PRIO_NORMAL, // SIGPIPE
	SIG_PRIO_NORMAL, // SIGALRM
	SIG_PRIO_NORMAL, // SIGTERM
	SIG_PRIO_NORMAL, // SIGUSR1
	SIG_PRIO_NORMAL, // SIGUSR2
	SIG_PRIO_NORMAL, // SIGCHLD
	SIG_PRIO_HIGH, // SIGPWR
	SIG_PRIO_NORMAL, // SIGWINCH
	SIG_PRIO_NORMAL, // SIGURG
	SIG_PRIO_NORMAL, // obsolete
	SIG_PRIO_STOP, // SIGSTOP
	SIG_PRIO_STOP, // SIGTSTP
	SIG_PRIO_STOP, // SIGCONT
	SIG_PRIO_STOP, // SIGTTIN
	SIG_PRIO_STOP, // SIGTTOU
	SIG_PRIO_NORMAL, // SIGVTALRM
	SIG_PRIO_NORMAL, // obsolete
	SIG_PRIO_CORE, // SIGXCPU
	SIG_PRIO_CORE, // SIGXFSZ
	SIG_PRIO_NORMAL, // SIGCORE
	SIG_PRIO_NORMAL, // SIGLWP
	SIG_PRIO_NORMAL, // SIGAIO
};

int Priority(int signum)
{
	assert(0 <= signum && signum < SIG_MAX_NUM);
	if ( !signum )
		return -1;
	if ( SIG__NUM_DECLARED <= signum )
		return SIG_PRIO_NORMAL;
	return PRIORITIES[signum];
}

Queue::Queue()
{
	for ( int i = 1; i < SIG_MAX_NUM; i++ )
		pending[i] = false;
}

void Queue::Push(int signum)
{
	assert(0 < signum && signum < SIG_MAX_NUM);
	pending[signum] = true;
}

int Queue::Pop(int cursig)
{
	int best = 0;
	int bestprio = Priority(cursig);
	for ( int i = 1; i < SIG_MAX_NUM; i++ )
		if ( pending[i] && bestprio < Priority(i) )
		{
			best = i;
			bestprio = Priority(i);
		}
	pending[best] = false;
	return best;
}

void Dispatch(CPU::InterruptRegisters* regs, void* /*user*/)
{
	return CurrentThread()->HandleSignal(regs);
}

void Return(CPU::InterruptRegisters* regs, void* /*user*/)
{
	return CurrentThread()->HandleSigreturn(regs);
}

void Init()
{
	Interrupt::RegisterRawHandler(130, isr130, true);
	Interrupt::RegisterHandler(130, Dispatch, NULL);
	Interrupt::RegisterRawHandler(131, isr131, true);
	Interrupt::RegisterHandler(131, Return, NULL);
}

} // namespace Signal
} // namespace Sortix
