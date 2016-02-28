/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    termios/tcsendbreak.c
    Send a break for a specific duration.

*******************************************************************************/

#include <sys/syscall.h>

#include <termios.h>

DEFN_SYSCALL2(int, sys_tcsendbreak, SYSCALL_TCSENDBREAK, int, int);

int tcsendbreak(int fd, int duration)
{
	return sys_tcsendbreak(fd, duration);
}
