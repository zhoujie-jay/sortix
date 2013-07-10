/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    kthread.cpp
    Utility and synchronization mechanisms for kernel threads.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/worker.h>
#include <sortix/signal.h>
#include "signal.h"
#include "thread.h"
#include "scheduler.h"

namespace Sortix {

// The kernel thread needs another stack to delete its own stack.
static void kthread_do_kill_thread(void* user)
{
	Thread* thread = (Thread*) user;
	while ( thread->state != Thread::State::DEAD )
		kthread_yield();
	delete thread;
}

extern "C" void kthread_exit(void* /*param*/)
{
	Worker::Schedule(kthread_do_kill_thread, CurrentThread());
	Scheduler::ExitThread();
	__builtin_unreachable();
}

struct kthread_cond_elem
{
	kthread_cond_elem_t* next;
	volatile unsigned long woken;
};

extern "C" void kthread_cond_wait(kthread_cond_t* cond, kthread_mutex_t* mutex)
{
	kthread_cond_elem_t elem;
	elem.next = NULL;
	elem.woken = 0;
	if ( cond->last ) { cond->last->next = &elem; }
	if ( !cond->last ) { cond->first = &elem; }
	cond->last = &elem;
	while ( !elem.woken )
	{
		kthread_mutex_unlock(mutex);
		Scheduler::Yield();
		kthread_mutex_lock(mutex);
	}
}

extern "C" unsigned long kthread_cond_wait_signal(kthread_cond_t* cond,
                                                  kthread_mutex_t* mutex)
{
	if ( Signal::IsPending() )
		return 0;
	kthread_cond_elem_t elem;
	elem.next = NULL;
	elem.woken = 0;
	if ( cond->last ) { cond->last->next = &elem; }
	if ( !cond->last ) { cond->first = &elem; }
	cond->last = &elem;
	while ( !elem.woken )
	{
		if ( Signal::IsPending() )
		{
			if ( cond->first == &elem )
			{
				cond->first = elem.next;
				if ( cond->last == &elem )
					cond->last = NULL;
			}
			else
			{
				kthread_cond_elem_t* prev = cond->first;
				while ( prev->next != &elem )
					prev = prev->next;
				prev->next = elem.next;
				if ( cond->last == &elem )
					cond->last = prev;
			}
			// Note that the thread still owns the mutex.
			return 0;
		}
		kthread_mutex_unlock(mutex);
		Scheduler::Yield();
		kthread_mutex_lock(mutex);
	}
	return 1;
}

extern "C" void kthread_cond_signal(kthread_cond_t* cond)
{
	kthread_cond_elem_t* elem = cond->first;
	if ( !elem ) { return; }
	if ( !(cond->first = elem->next) ) { cond->last = NULL; }
	elem->next = NULL;
	elem->woken = 1;
}

extern "C" void kthread_cond_broadcast(kthread_cond_t* cond)
{
	while ( cond->first ) { kthread_cond_signal(cond); }
}

} // namespace Sortix
