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

    semaphore/sem_timedwait.c
    Lock a semaphore.

*******************************************************************************/

#include <errno.h>
#include <sched.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <time.h>
#include <timespec.h>

int sem_timedwait(sem_t* restrict sem, const struct timespec* restrict abstime)
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
		// TODO: Using CLOCK_REALTIME for this is bad as it is not monotonic. We
		//       need to enchance the semaphore API so a better clock can be
		//       used instead.
		if ( errno == EAGAIN )
		{
			struct timespec now;
			clock_gettime(CLOCK_REALTIME, &now);
			if ( timespec_le(*abstime, now) )
				errno = ETIMEDOUT;
		}

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
