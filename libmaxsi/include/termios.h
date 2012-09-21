/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	termios.h
	Defines values for termios.

*******************************************************************************/

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef	_TERMIOS_H
#define	_TERMIOS_H 1

#include <features.h>
#include <stddef.h>
#include <sortix/termios.h>

__BEGIN_DECLS

int tcgetwinsize(int fd, struct winsize* ws);

__END_DECLS

#endif

