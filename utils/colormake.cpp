/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    colormake.cpp
    Colors the output of the make program.

*******************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int MODE_NONE = 1;
const int MODE_MAKE = 2;
const int MODE_GCC = 3;
const int MODE_GCC_MSG = 4;

int main(int /*argc*/, char* argv[])
{
	int pipe_fds[2];
	if ( pipe(pipe_fds) )
		error(1, errno, "pipe");
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		error(1, errno, "fork");
	if ( !child_pid )
	{
		dup2(pipe_fds[1], 1);
		dup2(pipe_fds[1], 2);
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		argv[0] = (char*) "make";
		execvp(argv[0],  argv);
		if ( errno == ENOENT )
			fprintf(stderr, "It would seem make isn't installed.\n");
		error(127, errno, "%s", argv[0]);
	}
	dup2(pipe_fds[0], 0);
	close(pipe_fds[0]);
	close(pipe_fds[1]);
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	int mode = MODE_NONE;
	while ( 0 < (line_len = getline(&line, &line_size, stdin)) )
	{
		if ( strstr(line, "make") )
			mode = MODE_MAKE;
		if ( strstr(line, "gcc") || strstr(line, "g++") )
			mode = MODE_GCC;
		const char* color = "\e[m";
		int next_mode = MODE_NONE;
		if ( mode == MODE_MAKE )
			color = "\e[36m";
		if ( mode == MODE_GCC )
			color = "\e[32m",
			next_mode = MODE_GCC_MSG;
		if ( mode == MODE_GCC_MSG )
		{
			next_mode = mode;
			if ( strstr(line, "warning:") ||
			     strstr(line, "note:") ||
			     strstr(line, "In file included from") ||
			     strstr(line, "                 from") ||
			     strstr(line, "At global scope") ||
			     strstr(line, "At top level") ||
			     strstr(line, "In member function") ||
			     strstr(line, "In function") ||
			     line[0] == ' ' )
				color = "\e[33m";
			else if ( strstr(line, "error:") )
				color = "\e[31m";
			else
				next_mode = MODE_NONE;
		}
		printf("%s%s", color, line);
		fflush(stdout);
		mode = next_mode;
	}
	free(line);
	int status;
	waitpid(child_pid, &status, 0);
	return WEXITSTATUS(status);
}
