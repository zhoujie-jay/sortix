/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of Sortix libpthread.

    Sortix libpthread is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libpthread is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libpthread. If not, see <http://www.gnu.org/licenses/>.

    pthread_mutex_trylock.c++
    Attempts to lock a mutex.

*******************************************************************************/

#include <errno.h>
#include <pthread.h>

static const unsigned long UNLOCKED_VALUE = 0;
static const unsigned long LOCKED_VALUE = 1;

extern "C" int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
	if ( !__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED_VALUE, LOCKED_VALUE) )
	{
		if ( mutex->type == PTHREAD_MUTEX_RECURSIVE &&
		     (pthread_t) mutex->owner == pthread_self() )
				return mutex->recursion++, 0;
		return errno = EBUSY;
	}
	mutex->owner = (unsigned long) pthread_self();
	mutex->recursion = 0;
	return 0;
}
