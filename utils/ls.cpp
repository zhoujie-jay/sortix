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

	ls.cpp
	Lists directory contents.

*******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <dirent.h>
#include <sys/wait.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>

int ls(const char* path)
{
	DIR* dir = opendir(path);
	if ( !dir ) { error(2, errno, "%s", path); return 2; }

	// TODO: Hack until mountpoints work correctly.
	if ( strcmp(path, "/") == 0 ) { printf("bin\ndev\n"); }

	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		printf("%s\n", entry->d_name);
	}

	if ( derror(dir) )
	{
		error(2, errno, "readdir: %s", path);
	}

	closedir(dir);

	return 0;
}

int main(int argc, char* argv[])
{
	pid_t childpid = 0;
	if ( isatty(1) )
	{
		int pipes[2];
		if ( pipe(pipes) ) { perror("pipe"); return 1; }
		childpid = fork();
		if ( childpid < 0 ) { perror("fork"); return 1; }
		if ( childpid )
		{
			close(1);
			dup(pipes[1]);
			close(pipes[0]);
			close(pipes[1]);
		}
		else
		{
			close(0);
			dup(pipes[0]);
			close(pipes[0]);
			close(pipes[1]);
			const char* columner = "column";
			const char* argv[] = { columner, NULL };
			execv(columner, (char* const*) argv);
			error(127, errno, "%s", columner);
		}
	}

	const size_t CWD_SIZE = 512;
	char cwd[CWD_SIZE];
	const char* path = getcwd(cwd, CWD_SIZE);
	if ( !path ) { path = "."; }

	if ( 1 < argc ) { path = argv[1]; }

	int result = ls(path);

	int status;
	if ( childpid ) { close(1); wait(&status); }
	return result;
}

