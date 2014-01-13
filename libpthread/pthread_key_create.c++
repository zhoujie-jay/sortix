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

    pthread_key_create.c++
    Thread-specific data key creation.

*******************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

static void noop_destructor(void*)
{
}

extern "C"
int pthread_key_create(pthread_key_t* key, void (*destructor)(void*))
{
	if ( !destructor )
		destructor = noop_destructor;

	pthread_mutex_lock(&__pthread_keys_lock);

	if ( __pthread_keys_used < __pthread_keys_length )
	{
		size_t result_key = 0;
		while ( __pthread_keys[result_key].destructor )
			result_key++;
		__pthread_keys[__pthread_keys_used++, *key = result_key].destructor = destructor;
	}
	else
	{
		size_t old_length = __pthread_keys_length;
		size_t new_length = old_length ? old_length * 2 : 8;
		size_t new_size = sizeof(struct pthread_key) * new_length;
		struct pthread_key* new_keys =
			(struct pthread_key*) realloc(__pthread_keys, new_size);
		if ( !new_keys )
		{
			pthread_mutex_unlock(&__pthread_keys_lock);
			return errno;
		}
		__pthread_keys = new_keys;
		__pthread_keys_length = new_length;
		for ( size_t i = old_length; i < new_length; i++ )
			new_keys[i].destructor = NULL;

		__pthread_keys[__pthread_keys_used++, *key = old_length].destructor = destructor;
	}

	pthread_mutex_unlock(&__pthread_keys_lock);

	return 0;
}
