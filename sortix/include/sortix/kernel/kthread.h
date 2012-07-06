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

	kthread.h
	Fake header providing noop threading functions. This is simply forward
	compatibility with the upcoming kthread branch and to ease merging.

*******************************************************************************/

#ifndef SORTIX_KTHREAD_H
#define SORTIX_KTHREAD_H

#define GOT_FAKE_KTHREAD
#warning Using noop kthread functions

namespace Sortix {

extern "C" {

typedef unsigned kthread_mutex_t;
const kthread_mutex_t KTHREAD_MUTEX_INITIALIZER = 0;
unsigned kthread_mutex_trylock(kthread_mutex_t* mutex);
void kthread_mutex_lock(kthread_mutex_t* mutex);
unsigned long kthread_mutex_lock_signal(kthread_mutex_t* mutex);
void kthread_mutex_unlock(kthread_mutex_t* mutex);
typedef unsigned kthread_cond_t;
const kthread_cond_t KTHREAD_COND_INITIALIZER = 0;
void kthread_cond_wait(kthread_cond_t* cond, kthread_mutex_t* mutex);
unsigned long kthread_cond_wait_signal(kthread_cond_t* cond, kthread_mutex_t* mutex);
void kthread_cond_signal(kthread_cond_t* cond);
void kthread_cond_broadcast(kthread_cond_t* cond);

} // extern "C"

class ScopedLock
{
public:
	ScopedLock(kthread_mutex_t* mutex)
	{
		this->mutex = mutex;
		kthread_mutex_lock(mutex);
	}

	~ScopedLock()
	{
		kthread_mutex_unlock(mutex);
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
		this->acquired = kthread_mutex_lock_signal(mutex);
	}

	~ScopedLockSignal()
	{
		if ( acquired )
			kthread_mutex_unlock(mutex);
	}

	bool IsAcquired() { return acquired; }

private:
	kthread_mutex_t* mutex;
	bool acquired;

};

} // namespace Sortix

#endif

