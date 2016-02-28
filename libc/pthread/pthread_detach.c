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

    pthread/pthread_detach.c
    Detach a thread.

*******************************************************************************/

#include <assert.h>
#include <pthread.h>

int pthread_detach(pthread_t thread)
{
	if ( pthread_mutex_trylock(&thread->detach_lock) != 0 )
		return pthread_join(thread, NULL);
	assert(thread->detach_state == PTHREAD_CREATE_JOINABLE);
	thread->detach_state = PTHREAD_CREATE_DETACHED;
	pthread_mutex_unlock(&thread->detach_lock);
	return 0;
}
