/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    kill.cpp
    Send a signal to a process.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int usage()
{
	printf("usage: kill [-n] pid ...\n");
	return 0;
}

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { return usage(); }

	int first = 1;
	int signum = SIGTERM;
	if ( argv[1][0] == '-' )
	{
		signum = atoi(argv[first++]);
		if ( argc < 3 ) { return usage(); }
	}

	int result = 0;

	for ( int i = first; i < argc; i++ )
	{
		pid_t pid = atoi(argv[i]);
		if ( kill(pid, signum) )
		{
			error(0, errno, "(%ji)", (intmax_t) pid);
			result |= 1;
		}
	}

	return result;
}
