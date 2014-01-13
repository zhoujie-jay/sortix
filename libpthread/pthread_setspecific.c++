/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    pthread_setspecific.c++
    Thread-specific data management.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

extern "C" int pthread_setspecific(pthread_key_t key, const void* value_const)
{
	void* value = (void*) value_const;

	struct pthread* thread = pthread_self();

	if ( key < thread->keys_length )
		return thread->keys[key] = value, 0;

	pthread_mutex_lock(&__pthread_keys_lock);

	assert(key < __pthread_keys_length);
	assert(__pthread_keys[key].destructor);

	pthread_mutex_unlock(&__pthread_keys_lock);

	size_t old_length = thread->keys_length;
	size_t new_length = __pthread_keys_length;
	size_t new_size = new_length * sizeof(void*);
	void** new_keys = (void**) realloc(thread->keys, new_size);
	if ( !new_keys )
		return errno;
	thread->keys = new_keys;
	thread->keys_length = new_length;
	for ( size_t i = old_length; i < new_length; i++ )
		new_keys[i] = NULL;

	return thread->keys[key] = value, 0;
}
