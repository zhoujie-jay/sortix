/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Tix.

    Tix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Tix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Tix. If not, see <https://www.gnu.org/licenses/>.

    tix-rmpatch.cpp
    Removes files from the current source directory.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int fgetc_or_die(FILE* input, const char* input_path, size_t* line,
                 size_t* column)
{
	int result = fgetc(input);
	if ( result == '\n' )
		(*line)++, *column = 0;
	else
		(*column)++;
	if ( result == EOF && ferror(input) )
		error(1, errno, "read: `%s'", input_path);
	return result;
}

int fgetc_or_die_eof(FILE* input, const char* input_path, size_t* line,
                     size_t* column)
{
	int result = fgetc_or_die(input, input_path, line, column);
	if ( result == EOF )
		error(1, errno, "%s:%zu:%zu: unexpected end of file",
		      input_path, *line, *column);
	return result;
}

bool check_eof(FILE* input, const char* input_path)
{
	size_t line = 0;
	size_t column = 0;
	int c = fgetc_or_die(input, input_path, &line, &column);
	if ( c != EOF )
	{
		ungetc(c, input);
		return false;
	}
	return true;
}

void parse_fixed(const char* text, FILE* input, const char* input_path,
                 size_t* line, size_t* column)
{
	for ( size_t i = 0; text[i]; i++ )
	{
		int ic = fgetc_or_die(input, input_path, line, column);
		if ( ic == EOF )
			error(1, errno, "%s:%zu:%zu: unexpected end of file, expected `%s'",
			      input_path, *line, *column, text + i);
		if ( ic != (unsigned char) text[i] )
			error(1, errno, "%s:%zu:%zu: parse error, expected `%s'", input_path,
			      *line, *column, text + i);
	}
}

bool rmpatch(FILE* input, const char* input_path, bool check)
{
	char* buffer = NULL;
	size_t buffer_used = 0;
	size_t buffer_length = 0;

	bool result = true;
	size_t line = 1;
	while ( !check_eof(input, input_path) )
	{
		buffer_used = 0;
		size_t column = 0;
		parse_fixed("rm -rf -- '", input, input_path, &line, &column);
		while ( true )
		{
			int ic = fgetc_or_die_eof(input, input_path, &line, &column);
			if ( ic == '\'' )
			{
				ic = fgetc_or_die(input, input_path, &line, &column);
				if ( ic == EOF || ic == '\n' )
					break;
				ungetc(ic, input);
				parse_fixed("\\''", input, input_path, &line, &column);
				ic = '\'';
			}
			if ( buffer_used == buffer_length )
			{
				size_t new_length = buffer_length ? 2 * buffer_length : 16;
				buffer = (char*) realloc(buffer, sizeof(char) * (new_length + 1));
				buffer_length = new_length;
			}
			buffer[buffer_used++] = ic;
		}
		if ( !buffer_used )
			error(1, errno, "%s:%zu: unexpected empty path", input_path, line);
		assert(buffer_length);
		buffer[buffer_used] = '\0';
		if ( buffer[0] == '/' )
			error(1, errno, "%s:%zu: unexpected absolute path", input_path, line);
		if ( does_path_contain_dotdot(buffer) )
			error(1, errno, "%s:%zu: unexpected path with ..", input_path, line);
		if ( check )
			continue;
		if ( pid_t child_pid = fork_or_death() )
		{
			int status;
			waitpid(child_pid, &status, 0);
			if ( WIFSIGNALED(status) )
				error(128 + WTERMSIG(status), 0, "child with pid %ju was killed by "
					  "signal %i (%s).", (uintmax_t) child_pid, WTERMSIG(status),
				      strsignal(WTERMSIG(status)));
			if ( WIFEXITED(status) && WEXITSTATUS(status) != 0 )
				result = false;
			continue;
		}
		const char* cmd_argv[] =
		{
			"rm",
			"-rf",
			"--",
			buffer,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	return result;
}

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [PATCH]\n", argv0);
	fprintf(fp, "Removes files from the current source directory.\n");
}

void version(FILE* fp, const char* argv0)
{
	help(fp, argv0);
}

int main(int argc, char* argv[])
{
	bool check = false;
	char* directory = NULL;

	const char* argv0 = argv[0];
	for ( int i = 0; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'c': check = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- `%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") ) { help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { version(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--check") ) { check = true; }
		else if ( GET_OPTION_VARIABLE("--directory", &directory) ) { }
		else
		{
			fprintf(stderr, "%s: unknown option: `%s'\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	CompactArguments(&argc, &argv);

	if ( 2 < argc )
	{
		fprintf(stderr, "%s: unexpected extra operand `%s'\n", argv0, argv[2]);
		help(stderr, argv0);
		exit(1);
	}

	const char* input_path = "<stdin>";
	FILE* input = stdin;

	if ( argc == 2 )
	{
		input_path = argv[1];
		if ( !(input = fopen(input_path, "r")) )
			error(1, errno, "`%s'", input_path);
	}

	if ( directory && chdir(directory) != 0 )
		error(1, errno, "chdir: `%s'", directory);
	free(directory);

	return rmpatch(input, input_path, check) ? 0 : 1;
}