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

    pthread_mutex_unlock.c++
    Unlocks a mutex.

*******************************************************************************/

#include <pthread.h>

static const unsigned long UNLOCKED_VALUE = 0;
static const unsigned long LOCKED_VALUE = 1;

extern "C" int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	if ( mutex->type == PTHREAD_MUTEX_RECURSIVE && mutex->recursion )
		return mutex->recursion--, 0;
	mutex->owner = 0;
	mutex->lock = UNLOCKED_VALUE;
	return 0;
}
