/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    signal/sigaction.cpp
    Set the action that happens when a signal is triggered.

*******************************************************************************/

#include <stdio.h>
#include <signal.h>

const int MAX_SIGNALS = 128;
extern sighandler_t handlers[MAX_SIGNALS];

// TODO: Actually implement the sigaction interface for real.
int sigaction(int signum, const struct sigaction* restrict act,
              struct sigaction* restrict oldact)
{
	if ( !act )
	{
		// TODO: set oldact->sa_mask here?
		oldact->sa_sigaction = NULL;
		oldact->sa_handler = handlers[signum];
		oldact->sa_flags = 0;
		return 0;
	}
	int understood_flags = SA_SIGINFO | SA_RESTART;
	if ( act->sa_flags & ~understood_flags )
	{
		fprintf(stderr, "%s:%u sigaction with unsupported flags 0x%x, ignoring "
		                "hoping they aren't needed.\n", __FILE__, __LINE__,
		                act->sa_flags & ~understood_flags);
	}
	if ( act->sa_flags & SA_RESTART )
		/* TODO: Actually implement this. Signals are a bit rare on Sortix right
		         now, so it doesn't matter much that system calls don't restart
		         on Sortix, so pretend that they do. */ {};
	if ( act->sa_flags & SA_SIGINFO )
	{
		fprintf(stderr, "%s:%u sigaction with unsupported SA_SIGINFO, ignoring "
		                "hoping the signal never happens.\n", __FILE__,
		                __LINE__);
		return 0;
	}
	sighandler_t new_handler = act->sa_handler;
	sighandler_t old_handler = signal(signum, new_handler);
	if ( old_handler == SIG_ERR )
		return -1;
	if ( oldact )
	{
		// TODO: set oldact->sa_mask here?
		oldact->sa_sigaction = NULL;
		oldact->sa_handler = old_handler;
		oldact->sa_flags = 0;
	}
	return 0;
}
