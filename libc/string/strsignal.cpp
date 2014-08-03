/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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
	case SIGHUP: return "Hangup";
	case SIGINT: return "Interrupt";
	case SIGQUIT: return "Quit";
	case SIGILL: return "Invalid instruction";
	case SIGTRAP: return "Trace/breakpoint trap";
	case SIGABRT: return "Aborted";
	case SIGBUS: return "Bus Error";
	case SIGFPE: return "Floating point exception";
	case SIGKILL: return "Killed";
	case SIGUSR1: return "User defined signal 1";
	case SIGSEGV: return "Segmentation fault";
	case SIGUSR2: return "User defined signal 2";
	case SIGPIPE: return "Broken pipe";
	case SIGALRM: return "Alarm clock";
	case SIGTERM: return "Terminated";
	case SIGSYS: return "Bad system call";
	case SIGCHLD: return "Child exited";
	case SIGCONT: return "Continued";
	case SIGSTOP: return "Stopped (signal)";
	case SIGTSTP: return "Stopped";
	case SIGTTIN: return "Stopped (tty input)";
	case SIGTTOU: return "Stopped (tty output)";
	case SIGURG: return "Urgent I/O condition";
	case SIGXCPU: return "CPU time limit exceeded";
	case SIGXFSZ: return "File size limit exceeded";
	case SIGVTALRM: return "Virtual timer expired";
	case SIGPWR: return "Power Fail/Restart";
	case SIGWINCH: return "Window changed";
	default: break;
	}

	if ( SIGRTMIN <= signum && signum <= SIGRTMAX )
		return "Real-time signal";

	return "Unknown signal value";
}

extern "C" char* strsignal(int signum)
{
	return (char*) sortix_strsignal(signum);
}
