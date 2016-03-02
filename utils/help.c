/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * help.c
 * Prints a friendly message and print the names of everything in PATH.
 */

#include <sys/stat.h>
#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
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
		struct dirent* entry;
		while ( (entry = readdir(dir)) )
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
