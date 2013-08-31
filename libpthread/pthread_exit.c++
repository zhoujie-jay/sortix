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
void pthread_exit(void* /*return_value*/)
{
	struct pthread* thread = pthread_self();
	size_t num_threads = 1;
	if ( num_threads == 1 )
		exit(0);
	struct exit_thread extended;
	memset(&extended, 0, sizeof(extended));
	extended.unmap_from = thread->uthread.stack_mmap;
	extended.unmap_size = thread->uthread.stack_size;
	exit_thread(0, EXIT_THREAD_UNMAP, &extended);
	__builtin_unreachable();
}
