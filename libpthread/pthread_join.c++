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

    pthread_join.c++
    Wait for thread termination.

*******************************************************************************/

#include <sys/mman.h>

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" int pthread_join(pthread_t thread, void** result_ptr)
{
	pthread_mutex_lock(&thread->join_lock);
	pthread_mutex_unlock(&thread->join_lock);
	void* result = thread->exit_result;
	munmap(thread->uthread.tls_mmap, thread->uthread.tls_size);
	if ( result_ptr )
		*result_ptr = result;
	return 0;
}
