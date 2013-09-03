/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    pthread_rwlock_tryrdlock.c++
    Attempts to acquire read access to a read-write lock.

*******************************************************************************/

#include <errno.h>
#include <pthread.h>

extern "C" int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock)
{
	if ( int ret = pthread_mutex_trylock(&rwlock->request_mutex) )
		return errno = ret;
	while ( rwlock->num_writers || rwlock->pending_writers )
	{
		pthread_mutex_unlock(&rwlock->request_mutex);
		return errno = EBUSY;
	}
	rwlock->num_readers++;
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
