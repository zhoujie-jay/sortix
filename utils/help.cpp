/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    help.cpp
    Prints a friendly message and print the names of everything in PATH.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int /*argc*/, char* /*argv*/[])
{
	printf("Please enter the name of one of the following programs:\n");

	const char* path = getenv("PATH");
	if ( !path )
		return 0;

	pid_t child_pid = -1;

	if ( isatty(1) )
	{
		int pipe_fds[2];
		if ( pipe(pipe_fds) )
			error(1, errno, "pipe");

		if ( (child_pid = fork()) < 0 )
			error(1, errno, "fork");

		if ( !child_pid )
		{
			dup2(pipe_fds[0], 0);
			close(pipe_fds[0]);
			close(pipe_fds[1]);
			execlp("column", "column", NULL);
			execlp("cat", "cat", NULL);
			int ic;
			while ( (ic = fgetc(stdin)) != EOF )
				fputc(ic, stdout);
			exit(0);
		}

		dup2(pipe_fds[1], 1);
		close(pipe_fds[0]);
		close(pipe_fds[1]);
	}

	char* dirname = NULL;
	while ( *path )
	{
		if ( dirname ) { free(dirname); dirname = NULL; }
		size_t len = strcspn(path, ":");
		if ( !len ) { path++; continue; }
		dirname = (char*) malloc((len+1) * sizeof(char));
		if ( !dirname )
			error(1, errno, "malloc");
		memcpy(dirname, path, len * sizeof(char));
		dirname[len] = '\0';
		path += len + 1;
		DIR* dir = opendir(dirname);
		if ( !dir )
			continue;
		while ( struct dirent* entry = readdir(dir) )
		{
			if ( entry->d_name[0] == '.' )
				continue;
			struct stat st;
			if ( fstatat(dirfd(dir), entry->d_name, &st, 0) )
				continue;
			if ( !S_ISREG(st.st_mode) )
				continue;
			if ( !(st.st_mode & 0111) )
				continue;
			printf("%s\n", entry->d_name);
		}
		closedir(dir);
	}
	free(dirname);

	if ( 0 <= child_pid )
	{
		fflush(stdout);
		close(1);
		int exit_status;
		waitpid(child_pid, &exit_status, 0);
		if ( WIFSIGNALED(exit_status) )
			exit(128 + WTERMSIG(exit_status));
		if ( WIFEXITED(exit_status) && WEXITSTATUS(exit_status) )
			exit(WEXITSTATUS(exit_status));
	}

	return 0;
}
