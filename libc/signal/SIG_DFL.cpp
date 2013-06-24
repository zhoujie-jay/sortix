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

    signal/SIG_DFL.cpp
    Default signal handler.

*******************************************************************************/

#include <signal.h>
#include <stdlib.h>

static void Core(int signum)
{
	exit(128 + signum);
}

extern "C" void SIG_DFL(int signum)
{
	if ( signum == SIGHUP ) { exit(128 + signum); } else
	if ( signum == SIGINT ) { exit(128 + signum); } else
	if ( signum == SIGQUIT ) { Core(signum); } else
	if ( signum == SIGTRAP ) { Core(signum); } else
	if ( signum == SIGABRT ) { Core(signum); } else
	if ( signum == SIGEMT ) { Core(signum); } else
	if ( signum == SIGFPE ) { Core(signum); } else
	if ( signum == SIGKILL ) { exit(128 + signum); } else
	if ( signum == SIGBUS ) { Core(signum); } else
	if ( signum == SIGSEGV ) { Core(signum); } else
	if ( signum == SIGSYS ) { Core(signum); } else
	if ( signum == SIGPIPE ) { exit(128 + signum); } else
	if ( signum == SIGALRM ) { exit(128 + signum); } else
	if ( signum == SIGTERM ) { exit(128 + signum); } else
	if ( signum == SIGUSR1 ) { exit(128 + signum); } else
	if ( signum == SIGUSR2 ) { exit(128 + signum); } else
	if ( signum == SIGCHLD ) { /* Ignore this signal. */ } else
	if ( signum == SIGPWR ) { /* Ignore this signal. */ } else
	if ( signum == SIGWINCH ) { /* Ignore this signal. */ } else
	if ( signum == SIGURG ) { /* Ignore this signal. */ } else
	if ( signum == SIGCONT ) { /* Ignore this signal. */ } else
	if ( signum == SIGVTALRM ) { /* Ignore this signal. */ } else
	if ( signum == SIGXCPU ) { Core(signum); } else
	if ( signum == SIGXFSZ ) { Core(signum); } else
	if ( signum == SIGWAITING ) { /* Ignore this signal. */ } else
	if ( signum == SIGLWP ) { /* Ignore this signal. */ } else
	if ( signum == SIGAIO ) { /* Ignore this signal. */ } else
	{ /* Ignore this signal. */ }
}
