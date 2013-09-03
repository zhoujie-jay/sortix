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

    pthread_cond_wait.c++
    Waits on a condition.

*******************************************************************************/

#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

extern "C"
int pthread_cond_wait(pthread_cond_t* restrict cond,
                      pthread_mutex_t* restrict mutex)
{
	struct pthread_cond_elem elem;
	elem.next = NULL;
	elem.woken = 0;
	if ( cond->last )
		cond->last->next = &elem;
	if ( !cond->last )
		cond->first = &elem;
	cond->last = &elem;
	while ( !elem.woken )
	{
		pthread_mutex_unlock(mutex);
		sched_yield();
		pthread_mutex_lock(mutex);
	}
	return 0;
}
