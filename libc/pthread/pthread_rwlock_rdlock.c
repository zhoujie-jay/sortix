/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    pthread/pthread_rwlock_rdlock.c
    Acquires read access to a read-write lock.

*******************************************************************************/

#include <pthread.h>

int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock)
{
	pthread_mutex_lock(&rwlock->request_mutex);
	rwlock->pending_readers++;
	while ( rwlock->num_writers || rwlock->pending_writers )
		pthread_cond_wait(&rwlock->reader_condition, &rwlock->request_mutex);
	rwlock->pending_readers--;
	rwlock->num_readers++;
	pthread_mutex_unlock(&rwlock->request_mutex);
	return 0;
}
