/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * semaphore/sem_timedwait.c
 * Lock a semaphore.
 */

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
