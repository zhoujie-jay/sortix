/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    signal/signal.cpp
    Configure and retrieve a signal handler.

*******************************************************************************/

#include <signal.h>
#include <string.h>

extern "C" void (*signal(int signum, void (*handler)(int)))(int)
{
	// Create a structure describing the new handler.
	struct sigaction newact;
	memset(&newact, 0, sizeof(newact));
	sigemptyset(&newact.sa_mask);
	newact.sa_handler = handler;
	newact.sa_flags = SA_RESTART;

	// Register the new handler and atomically get the old.
	struct sigaction oldact;
	if ( sigaction(signum, &newact, &oldact) != 0 )
		return SIG_ERR;

	// We can't return the old handler properly if it's SA_SIGINFO or SA_COOKIE,
	// unless it's the common SIG_IGN or SIG_DFL handlers. Let's just say to the
	// caller that it's SIG_DFL and assume that they'll be using sigaction
	// instead if they wish to restore an old handler.
	if ( (oldact.sa_flags & (SA_SIGINFO | SA_COOKIE)) &&
	     oldact.sa_handler != SIG_IGN &&
	     oldact.sa_handler != SIG_DFL )
		return SIG_DFL;

	return oldact.sa_handler;
}
