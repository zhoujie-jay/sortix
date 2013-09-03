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

    pthread_rwlock_wrlock.c++
    Acquires write access to a read-write lock.

*******************************************************************************/

#include <pthread.h>

extern "C" int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock)
{
	pthread_mutex_lock(&rwlock->request_mutex);
	rwlock->pending_writers++;
	while ( rwlock->num_readers || rwlock->num_writers )
		pthread_cond_wait(&rwlock->writer_condition, &rwlock->request_mutex);
	rwlock->pending_writers--;
	rwlock->num_writers = 1;
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
