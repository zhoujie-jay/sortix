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

    pthread_exit.c++
    Exits the current thread.

*******************************************************************************/

#include <sys/mman.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern "C"
__attribute__((__noreturn__))
void pthread_exit(void* return_value)
{
	struct pthread* thread = pthread_self();

	pthread_mutex_lock(&__pthread_keys_lock);
	bool keys_left = true;
	while ( keys_left )
	{
		keys_left = false;
		for ( pthread_key_t key = 0; key < thread->keys_length; key++ )
		{
			void* key_value = thread->keys[key];
			if ( !key_value )
				continue;
			thread->keys[key] = NULL;
			if ( __pthread_keys[key].destructor )
				__pthread_keys[key].destructor(key_value);
			keys_left = true;
		}
	}
	free(thread->keys);
	thread->keys = NULL;
	thread->keys_length = 0;
	pthread_mutex_unlock(&__pthread_keys_lock);

	pthread_mutex_lock(&thread->detach_lock);
	thread->exit_result = return_value;
	int exit_flags = EXIT_THREAD_UNMAP;
	struct exit_thread extended;
	memset(&extended, 0, sizeof(extended));
	extended.unmap_from = thread->uthread.stack_mmap;
	extended.unmap_size = thread->uthread.stack_size;
	if ( thread->detach_state == PTHREAD_CREATE_JOINABLE )
	{
		extended.zero_from = &thread->join_lock.lock;
		extended.zero_size = sizeof(thread->join_lock.lock);
		exit_flags |= EXIT_THREAD_ZERO;
	}
	else
	{
		extended.tls_unmap_from = thread->uthread.tls_mmap;
		extended.tls_unmap_size = thread->uthread.tls_size;
		exit_flags |= EXIT_THREAD_TLS_UNMAP;
	}

	exit_thread(0, EXIT_THREAD_ONLY_IF_OTHERS | exit_flags, &extended);
	exit(0);
}
