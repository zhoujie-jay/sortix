/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

#include "platform.h"
#include <libmaxsi/memory.h>
#include "panic.h"
#include "signal.h"

using namespace Maxsi;

namespace Sortix
{
	const int PRIORITY_NORMAL = 0;
	const int PRIORITY_HIGH = 1;
	const int PRIORITY_STOP = 2;
	const int PRIORITY_CORE = 3;
	const int PRIORITY_KILL = 4;

	const int PRIORITIES[Maxsi::Signal::NUMSIGNALS] =
	{
		PRIORITY_NORMAL, // unused
		PRIORITY_NORMAL, // SIGHUP
		PRIORITY_NORMAL, // SIGINT
		PRIORITY_NORMAL, // SIGQUIT
		PRIORITY_CORE, // SIGILL
		PRIORITY_CORE, // SIGTRAP
		PRIORITY_CORE, // SIGABRT
		PRIORITY_CORE, // SIGEMT
		PRIORITY_CORE, // SIGFPE
		PRIORITY_KILL, // SIGKILL
		PRIORITY_CORE, // SIGBUS
		PRIORITY_CORE, // SIGSEGV
		PRIORITY_CORE, // SIGSYS
		PRIORITY_NORMAL, // SIGPIPE
		PRIORITY_NORMAL, // SIGALRM
		PRIORITY_NORMAL, // SIGTERM
		PRIORITY_NORMAL, // SIGUSR1
		PRIORITY_NORMAL, // SIGUSR2
		PRIORITY_NORMAL, // SIGCHLD
		PRIORITY_HIGH, // SIGPWR
		PRIORITY_NORMAL, // SIGWINCH
		PRIORITY_NORMAL, // SIGURG
		PRIORITY_NORMAL, // obsolete
		PRIORITY_STOP, // SIGSTOP
		PRIORITY_STOP, // SIGTSTP
		PRIORITY_STOP, // SIGCONT
		PRIORITY_STOP, // SIGTTIN
		PRIORITY_STOP, // SIGTTOU
		PRIORITY_NORMAL, // SIGVTALRM
		PRIORITY_NORMAL, // obsolete
		PRIORITY_CORE, // SIGXCPU
		PRIORITY_CORE, // SIGXFSZ
		PRIORITY_NORMAL, // SIGCORE
		PRIORITY_NORMAL, // SIGLWP
		PRIORITY_NORMAL, // SIGAIO
	};

	// Returns true of the exact ordering of this signal version others aren't
	// important - if it returns false, then this signal will be put in the
	// queue according to priority, instead of being merged into another signal
	// with the same signum.
	bool Unifiable(int /*signum*/)
	{
		return true;
	}

	// Returns whether a specific signal is more important to deliver than
	// another. This is used to schedule signals.
	int CompareSignalPriority(int siga, int sigb)
	{
		int prioa = PRIORITY_NORMAL;
		int priob = PRIORITY_NORMAL;

		if ( siga < Maxsi::Signal::NUMSIGNALS ) { prioa = PRIORITIES[siga]; }
		if ( sigb < Maxsi::Signal::NUMSIGNALS ) { priob = PRIORITIES[sigb]; }

		if ( prioa < priob ) { return -1; } else
		if ( prioa > priob ) { return 1; }
		return 0;
	}

	SignalQueue::SignalQueue()
	{
		queue = NULL;
	}

	SignalQueue::~SignalQueue()
	{
		while ( queue )
		{
			Signal* todelete = queue;
			queue = queue->nextsignal;
			delete todelete;
		}
	}

	// Queues the signal and schedules it for processing.
	bool SignalQueue::Push(int signum)
	{
		ASSERT(0 <= signum && signum < 128);

		if ( Unifiable(signum) )
		{
			for ( Signal* signal = queue; signal != NULL; signal = signal->nextsignal )
			{
				if ( signal->signum != signum ) { continue; }

				signal->numpending++;
				return true;
			}
		}

		Signal* signal = new Signal;
		if ( !signal ) { return false; }

		signal->signum = signum;
		signal->numpending = 1;
		signal->nextsignal = NULL;
		signal->returncode = 128 + signum;

		Insert(signal);

		return true;
	}

	// Insert the signal in O(N), which is pretty fast for small Ns.
	void SignalQueue::Insert(Signal* signal)
	{
		if ( !queue )
		{
			queue = signal;
			last = signal;
			return;
		}

		// If the signal is to be inserted last, then just do it quickly.
		if ( last != NULL && 0 <= CompareSignalPriority(last->signum, signal->signum) )
		{
			last->nextsignal = signal;
			signal->nextsignal = NULL;
			last = signal;
			return;
		}

		// Check if the signal should be inserted first.
		if ( queue != NULL && CompareSignalPriority(queue->signum, signal->signum) < 0 )
		{
			signal->nextsignal = queue;
			queue = signal;
			return;
		}

		// Find where the signal should be inserted.
		for ( Signal* tmp = queue; tmp != NULL; tmp = tmp->nextsignal )
		{
			Signal* next = tmp->nextsignal;

			if ( next != NULL && CompareSignalPriority(next->signum, signal->signum) < 0 )
			{
				tmp->nextsignal = signal;
				signal->nextsignal = next;
				return;
			}

			if ( next == NULL )
			{
				tmp->nextsignal = signal;
				signal->nextsignal = NULL;
				last = signal;
				return;
			}
		}
	}

	// Given the stack of currently processing signals, return a new signal if
	// it is more important to handle at this point.
	Signal* SignalQueue::Pop(Signal* current)
	{
		if ( queue == NULL ) { return NULL; }

		bool returnqueue = false;

		// If we are currently handling no signal, then just return the first.
		if ( current == NULL ) { returnqueue = true; }

		// If we are handling a signal, only override it with another if it is
		// more important.
		else if ( CompareSignalPriority(current->signum, queue->signum) < 0 )
		{
			returnqueue = true;
		}

		if ( returnqueue )
		{
			Signal* result = queue;
			queue = queue->nextsignal;
			result->nextsignal = NULL;
			return result;
		}

		return NULL;
	}

	Signal* Signal::Fork()
	{
		Signal* clone = new Signal();
		if ( !clone ) { return NULL; }

		Memory::Copy(clone, this, sizeof(Signal));

		Signal* nextsignalclone = NULL;
		if ( nextsignal )
		{
			nextsignalclone = nextsignal->Fork();
			if ( !nextsignalclone ) { delete clone; return NULL; }
		}

		clone->nextsignal = nextsignalclone;

		return clone;
	}
}

