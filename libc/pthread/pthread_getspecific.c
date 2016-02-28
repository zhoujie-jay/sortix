/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    pthread/pthread_getspecific.c
    Thread-specific data management.

*******************************************************************************/

#include <assert.h>
#include <pthread.h>

void* pthread_getspecific(pthread_key_t key)
{
	struct pthread* thread = pthread_self();

	if ( key < thread->keys_length )
		return thread->keys[key];

	pthread_mutex_lock(&__pthread_keys_lock);

	assert(key < __pthread_keys_length);
	assert(__pthread_keys[key].destructor);

	pthread_mutex_unlock(&__pthread_keys_lock);

	return NULL;
}
