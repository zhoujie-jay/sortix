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

    sem_wait.c++
    Lock a semaphore.

*******************************************************************************/

#include <errno.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>

extern "C" int sem_wait(sem_t* sem)
{
	if ( sem_trywait(sem) == 0 )
		return 0;
	if ( errno != EAGAIN )
		return -1;

	sigset_t old_set_mask;
	sigset_t old_set_allowed;
	sigset_t all_signals;
	sigfillset(&all_signals);
	sigprocmask(SIG_SETMASK, &all_signals, &old_set_mask);
	signotset(&old_set_allowed, &old_set_mask);

	while ( sem_trywait(sem) != 0 )
	{
		if ( errno == EAGAIN && sigpending(&old_set_allowed) )
			errno = EINTR;

		if ( errno != EAGAIN )
		{
			sigprocmask(SIG_SETMASK, &old_set_mask, NULL);
			return -1;
		}

		sched_yield();
	}

	sigprocmask(SIG_SETMASK, &old_set_mask, NULL);

	return 0;
}
