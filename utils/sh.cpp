/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sh.cpp
    Forward execution to the best shell.

*******************************************************************************/

#include <sys/wait.h>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char* getenv_safe(const char* name, const char* def = "")
{
	const char* ret = getenv(name);
	return ret ? ret : def;
}

bool is_existing_shell(const char* candidate)
{
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return false;
	if ( !child_pid )
	{
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
		execlp("which", "which", "--", candidate, (char*) NULL);
		exit(127);
	}
	int status;
	waitpid(child_pid, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

char* search_for_proper_shell()
{
	if ( getenv("SORTIX_SH_BACKEND") )
	{
		if ( !getenv("SORTIX_SH_BACKEND")[0] )
			return NULL;
		if ( is_existing_shell(getenv("SORTIX_SH_BACKEND")) )
			return strdup(getenv("SORTIX_SH_BACKEND"));
	}

	const char* backends_dir_path =
		getenv_safe("SORTIX_SH_BACKENDS_DIR", "/etc/proper-shells");

	struct dirent** shell_entries;
	int num_shell_entries = scandir(backends_dir_path, &shell_entries, NULL, alphasort);
	if ( 0 <= num_shell_entries )
	{
		char* result = NULL;
		for ( int i = 0; i < num_shell_entries; i++ )
		{
			struct dirent* entry = shell_entries[i];
			const char* name = entry->d_name;
			if ( !strcmp(name, ".") || !strcmp(name, "..") )
				continue;
			size_t path_length = strlen(backends_dir_path) + 1 + strlen(name);
			char* path = (char*) malloc(path_length + 1);
			if ( !path )
				continue;
			stpcpy(stpcpy(stpcpy(path, backends_dir_path), "/"), name);
			FILE* fp = fopen(path, "r");
			free(path);
			if ( !fp )
				continue;
			size_t result_size = 0;
			ssize_t result_length = getline(&result, &result_size, fp);
			fclose(fp);
			if ( result_length < 0 )
				continue;
			if ( result_length && result[result_length-1] == '\n' )
				result[--result_length] = '\0';
			if ( !is_existing_shell(result) )
			{
				free(result);
				result = NULL;
				continue;
			}
			break;
		}
		for ( int i = 0; i < num_shell_entries; i++ )
			free(shell_entries[i]);
		free(shell_entries);
		if ( result )
			return result;
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	if ( argc == 1 && isatty(0) && isatty(1) )
		execvp("sortix-sh", argv);

	char* proper_shell = search_for_proper_shell();
	if ( proper_shell )
		execvp(proper_shell, argv);
	free(proper_shell);

	execvp("sortix-sh", argv);
	return 127;
}
