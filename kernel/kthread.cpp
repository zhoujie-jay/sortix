/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * kthread.cpp
 * Utility and synchronization mechanisms for kernel threads.
 */

#include <sortix/signal.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/worker.h>

namespace Sortix {

// The kernel thread needs another stack to delete its own stack.
static void kthread_do_kill_thread(void* user)
{
	Thread* thread = (Thread*) user;
	while ( thread->state != ThreadState::DEAD )
		kthread_yield();
	FreeThread(thread);
}

extern "C" void kthread_exit()
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
