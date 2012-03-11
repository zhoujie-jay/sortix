/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	init.cpp
	Initializes the system by setting up the terminal and starting the shell.

*******************************************************************************/

#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>

int parent(pid_t childid)
{
	int status;
	waitpid(childid, &status, 0);
	return status;
}

int child()
{
	const char* programname = "sh";
	const char* newargv[] = { programname, NULL };

	execv(programname, (char* const*) newargv);
	error(0, errno, "%s", programname);

	return 2;
}

int main(int argc, char* argv[])
{
	if ( open("/dev/tty", O_RDONLY) != 0 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 1 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 2 ) { return 2; }

	// Reset the terminal's color and the rest of it.
	printf("\r\e[m\e[J");
	fflush(stdout);

	pid_t childpid = fork();	
	if ( childpid < 0 ) { perror("fork"); return 2; }

	return ( childpid == 0 ) ? child() : parent(childpid);
}

