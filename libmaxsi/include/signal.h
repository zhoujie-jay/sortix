/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	signal.h
	Signals.

*******************************************************************************/

/* TODO: This does not fully implement POSIX 2008-1 yet! */

#ifndef	_SIGNAL_H
#define	_SIGNAL_H 1

#include <features.h>
#include <sortix/signal.h>

__BEGIN_DECLS

@include(pid_t.h);

typedef void (*sighandler_t)(int);

void SIG_DFL(int signum);
void SIG_IGN(int signum);
void SIG_ERR(int signum);

sighandler_t signal(int signum, sighandler_t handler);
int kill(pid_t pid, int sig);
int raise(int sig);

__END_DECLS

#endif
