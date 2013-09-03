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

    pthread_rwlock_unlock.c++
    Releases hold of a read-write lock.

*******************************************************************************/

#include <pthread.h>

extern "C" int pthread_rwlock_unlock(pthread_rwlock_t* rwlock)
{
	pthread_mutex_lock(&rwlock->request_mutex);
	if ( rwlock->num_writers )
	{
		rwlock->num_writers = 0;
		if ( rwlock->pending_writers )
			pthread_cond_signal(&rwlock->writer_condition);
		else
			pthread_cond_broadcast(&rwlock->reader_condition);
	}
	else
	{
		if ( --rwlock->num_readers == 0 && rwlock->pending_writers )
			pthread_cond_signal(&rwlock->writer_condition);
	}
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
