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
 * sortix/kernel/kthread.h
 * Utility and synchronization mechanisms for kernel threads.
 */

#ifndef SORTIX_KTHREAD_H
#define SORTIX_KTHREAD_H

#include <stddef.h>

#include <sortix/signal.h>

namespace Sortix {

extern "C" {

inline static void kthread_yield(void) { asm volatile ("int $129"); }
__attribute__((noreturn)) void kthread_exit();
typedef unsigned kthread_mutex_t;
const kthread_mutex_t KTHREAD_MUTEX_INITIALIZER = 0;
unsigned kthread_mutex_trylock(kthread_mutex_t* mutex);
void kthread_mutex_lock(kthread_mutex_t* mutex);
unsigned long kthread_mutex_lock_signal(kthread_mutex_t* mutex);
void kthread_mutex_unlock(kthread_mutex_t* mutex);
struct kthread_cond_elem;
typedef struct kthread_cond_elem kthread_cond_elem_t;
struct kthread_cond
{
	kthread_cond_elem_t* first;
	kthread_cond_elem_t* last;
};
typedef struct kthread_cond kthread_cond_t;
const kthread_cond_t KTHREAD_COND_INITIALIZER = { NULL, NULL };
void kthread_cond_wait(kthread_cond_t* cond, kthread_mutex_t* mutex);
unsigned long kthread_cond_wait_signal(kthread_cond_t* cond, kthread_mutex_t* mutex);
void kthread_cond_signal(kthread_cond_t* cond);
void kthread_cond_broadcast(kthread_cond_t* cond);

} /* extern "C" */

class ScopedLock
{
public:
	ScopedLock(kthread_mutex_t* mutex)
	{
		this->mutex = mutex;
		if ( mutex )
			kthread_mutex_lock(mutex);
	}

	~ScopedLock()
	{
		Reset();
	}

	void Reset()
	{
		if ( mutex )
			kthread_mutex_unlock(mutex);
		mutex = NULL;
	}

private:
	kthread_mutex_t* mutex;

};

class ScopedLockSignal
{
public:
	ScopedLockSignal(kthread_mutex_t* mutex)
	{
		this->mutex = mutex;
		this->acquired = !mutex || kthread_mutex_lock_signal(mutex);
	}

	~ScopedLockSignal()
	{
		Reset();
	}

	void Reset()
	{
		if ( mutex && acquired )
			kthread_mutex_unlock(mutex);
		mutex = NULL;
	}

	bool IsAcquired() { return acquired; }

private:
	kthread_mutex_t* mutex;
	bool acquired;

};

} // namespace Sortix

#endif
