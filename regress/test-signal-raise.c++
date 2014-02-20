/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    test-signal-raise.c++
    Tests whether basic raise() support works.

*******************************************************************************/

#include <signal.h>

#include "test.h"

static int signal_counter = 0;

void signal_handler(int signum, siginfo_t* siginfo, void* ucontext_ptr)
{
	(void) signum;
	(void) siginfo;
	(void) ucontext_ptr;

	test_assert(signum == SIGUSR1);

	signal_counter++;
}

int main(void)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = signal_handler;
	sa.sa_flags = SA_SIGINFO;

	if ( sigaction(SIGUSR1, &sa, NULL) )
		test_error(errno, "sigaction(USR1)");

	raise(SIGUSR1);

	test_assert(signal_counter == 1);

	return 0;
}
