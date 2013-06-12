/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    string/strsignal.cpp
    Convert signal number to a string.

*******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0
#include <signal.h>
#include <string.h>

extern "C" const char* sortix_strsignal(int signum)
{
	switch ( signum )
	{
	case SIGHUP: return "SIGHUP";
	case SIGINT: return "SIGINT";
	case SIGQUIT: return "SIGQUIT";
	case SIGILL: return "SIGILL";
	case SIGTRAP: return "SIGTRAP";
	case SIGABRT: return "SIGABRT";
	case SIGEMT: return "SIGEMT";
	case SIGFPE: return "SIGFPE";
	case SIGKILL: return "SIGKILL";
	case SIGBUS: return "SIGBUS";
	case SIGSEGV: return "SYSSEGV";
	case SIGSYS: return "SIGSYS";
	case SIGPIPE: return "SIGPIPE";
	case SIGALRM: return "SIGALRM";
	case SIGTERM: return "SIGTERM";
	case SIGUSR1: return "SIGUSR1";
	case SIGUSR2: return "SIGUSR2";
	case SIGCHLD: return "SIGCHLD";
	case SIGPWR: return "SIGPWR";
	case SIGWINCH: return "SIGWINCH";
	case SIGURG: return "SIGURG";
	case SIGSTOP: return "SIGSTOP";
	case SIGTSTP: return "SIGTSTP";
	case SIGCONT: return "SIGCONT";
	case SIGTTIN: return "SIGTTIN";
	case SIGTTOU: return "SIGTTOU";
	case SIGVTALRM: return "SIGVTALRM";
	case SIGXCPU: return "SIGXCPU";
	case SIGXFSZ: return "SIGXFSZ";
	case SIGWAITING: return "SIGWAITING";
	case SIGLWP: return "SIGLWP";
	case SIGAIO: return "SIGAIO";
	default: return "Unknown signal value";
	}
}

extern "C" char* strsignal(int signum)
{
	return (char*) sortix_strsignal(signum);
}
