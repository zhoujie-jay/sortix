/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

#include <sys/stat.h>
#include <sys/types.h>
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
	pid_t init_pid = getppid();
	char init_pid_str[sizeof(pid_t)*3];
	snprintf(init_pid_str, sizeof(pid_t)*3, "%ju", (uintmax_t) init_pid);
	setenv("INIT_PID", init_pid_str, 1);

	setpgid(0, 0);

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
		while ( 0 < waitpid(-1, NULL, WNOHANG) );
		// TODO: Use the proper macro!
		if ( 128 <= WEXITSTATUS(status) || WIFSIGNALED(status) )
		{
			printf("Looks like the system crashed, trying to bring it back up.\n");
			return runsystem();
		}
		return WEXITSTATUS(status);
	}

	exit(child());
}

int main(int /*argc*/, char* /*argv*/[])
{
	if ( open("/dev/tty", O_RDONLY) != 0 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 1 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 2 ) { return 2; }

	// Reset the terminal's color and the rest of it.
	printf("\r\e[m\e[J");
	fflush(stdout);

	// Set the default file creation mask.
	umask(022);

	// By default, compile to the same architecture that the kernel told us that
	// we are running.
	setenv("objtype", getenv("cputype"), 0);

	// Set up the PATH variable.
	const char* prefix = "/";
	const char* cputype = getenv("cputype");
	const char* suffix = "/bin";
	char* path = new char[strlen(prefix) + strlen(cputype) + strlen(suffix) + 1];
	stpcpy(stpcpy(stpcpy(path, prefix), cputype), suffix);
	setenv("PATH", path, 0);
	delete[] path;

	// Make sure that we have a /tmp directory.
	mkdir("/tmp", 01777);

	return runsystem();
}
