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

	event.cpp
	Each thread can wait for an event to happen and be signaled when it does.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include "thread.h"
#include "syscall.h"
#include "event.h"

namespace Sortix
{
	Event::Event()
	{
		waiting = NULL;
	}

	Event::~Event()
	{
		if ( waiting )
		{
			Panic("Thread was waiting on event, but it went out of scope");
		}
	}

	void Event::Register()
	{
		Thread* thread = CurrentThread();
		if ( thread->event )
		{
			Panic("Thread tried to wait on an event, but was already waiting");
		}
		thread->event = this;
		thread->eventnextwaiting = waiting;
		waiting = thread;
	}

	void Event::Signal()
	{
		while ( waiting )
		{
			waiting->event = NULL;
			Syscall::ScheduleResumption(waiting);
			waiting = waiting->eventnextwaiting;
		}
	}

	// TODO: Okay, I realize this is O(N), refactor this to a linked list.
	void Event::Unregister(Thread* thread)
	{
		if ( thread->event != this ) { return; }
		thread->event = NULL;
		if ( waiting == thread ) { waiting = thread->eventnextwaiting; return; }
		for ( Thread* tmp = waiting; tmp; tmp = tmp->eventnextwaiting )
		{
			if ( tmp->eventnextwaiting == thread )
			{
				tmp->eventnextwaiting = thread->eventnextwaiting;
				break;
			}
		}
		thread->eventnextwaiting = NULL;
	}
}

