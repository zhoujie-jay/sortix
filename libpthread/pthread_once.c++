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

    pthread_once.c++
    Dynamic package initialization.

*******************************************************************************/

#include <pthread.h>

extern "C"
int pthread_once(pthread_once_t* once_control, void (*init_routine)(void))
{
	pthread_mutex_lock(&once_control->lock);
	if ( !once_control->executed )
	{
		// TODO: If this thread is a cancellation point and is canceled, we must
		//       pretend this pthread_once was never called. This means that we
		//       need to ensure that once_control->lock is unlocked in the event
		//       of a cancellation.
		init_routine();
		once_control->executed = 1;
	}
	pthread_mutex_unlock(&once_control->lock);
	return 0;
}
