/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

#warning Using noop kthread functions

namespace Sortix {

typedef unsigned kthread_mutex_t;
const kthread_mutex_t KTHREAD_MUTEX_INITIALIZER = 0;
extern "C" unsigned kthread_mutex_trylock(kthread_mutex_t* mutex);
extern "C" void kthread_mutex_lock(kthread_mutex_t* mutex);
extern "C" void kthread_mutex_unlock(kthread_mutex_t* mutex);
typedef unsigned kthread_cond_t;
const kthread_cond_t KTHREAD_COND_INITIALIZER = 0;
extern "C" void kthread_cond_wait(kthread_cond_t* cond, kthread_mutex_t* mutex);
extern "C" void kthread_cond_signal(kthread_cond_t* cond);
extern "C" void kthread_cond_broadcast(kthread_cond_t* cond);

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

} // namespace Sortix

#endif

