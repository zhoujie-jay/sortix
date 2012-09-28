/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	signal.cpp
	Handles the good old unix signals.

*******************************************************************************/

#include <sys/types.h>
#include <sys/syscall.h>
#include <signal.h>

const int MAX_SIGNALS = 128;
sighandler_t handlers[MAX_SIGNALS];

extern "C" void SignalHandlerAssembly(int signum);
extern "C" void SignalHandler(int signum)
{
	if ( 0 <= signum && signum < (int) MAX_SIGNALS )
		handlers[signum](signum);
}

DEFN_SYSCALL1_VOID(sys_register_signal_handler, SYSCALL_REGISTER_SIGNAL_HANDLER, sighandler_t);

extern "C" void init_signal()
{
	for ( int i = 0; i < MAX_SIGNALS; i++ )
		handlers[i] = SIG_DFL;

	// Tell the kernel which function we want called upon signals.
	sys_register_signal_handler(&SignalHandlerAssembly);
}

extern "C" sighandler_t signal(int signum, sighandler_t handler)
{
	if ( signum < 0 || MAX_SIGNALS <= signum ) { return SIG_ERR; }
	return handlers[signum] = handler;
}
