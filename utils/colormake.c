/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * colormake.c
 * Colors the output of the make program.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int MODE_NONE = 1;
static const int MODE_MAKE = 2;
static const int MODE_GCC = 3;
static const int MODE_GCC_MSG = 4;

int main(int argc, char* argv[])
{
	(void) argc;
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
		if ( strstr(line, "cc") || strstr(line, "c++") ||
		     strstr(line, "gcc") || strstr(line, "g++") )
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
	printf("\e[m");
	fflush(stdout);
	return WEXITSTATUS(status);
}
