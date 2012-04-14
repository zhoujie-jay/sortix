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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>

int child()
{
	const char* programname = "sh";
	const char* newargv[] = { programname, NULL };

	execvp(programname, (char* const*) newargv);
	error(0, errno, "%s", programname);

	return 2;
}

int runsystem()
{
	pid_t childpid = fork();
	if ( childpid < 0 ) { perror("fork"); return 2; }

	if ( childpid )
	{
		int status;
		waitpid(childpid, &status, 0);
		// TODO: Use the proper macro!
		if ( 128 <= status )
		{
			printf("Looks like the system crashed, trying to bring it back up.\n");
			return runsystem();
		}
		return status;
	}

	return child();
}

int main(int argc, char* argv[])
{
	if ( open("/dev/tty", O_RDONLY) != 0 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 1 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 2 ) { return 2; }

	// Reset the terminal's color and the rest of it.
	printf("\r\e[m\e[J");
	fflush(stdout);

	// By default, compile to the same architecture that the kernel told us that
	// we are running.
	setenv("objtype", getenv("cputype"), 0);

	return runsystem();
}

