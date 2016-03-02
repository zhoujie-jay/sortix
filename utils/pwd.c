/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * pwd.c
 * Prints the current working directory.
 */

#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool nul_or_slash(char c)
{
	return !c || c == '/';
}

static bool next_elem_is_dot_or_dot_dot(const char* path)
{
	return (path[0] == '.' && nul_or_slash(path[1])) ||
	       (path[0] == '.' && path[1] == '.' && nul_or_slash(path[2]));
}

static bool is_path_absolute(const char* path)
{
	size_t index = 0;
	if ( path[index++] != '/' )
		return false;
	while ( path[index] )
	{
		if ( next_elem_is_dot_or_dot_dot(path) )
			return false;
		while ( !nul_or_slash(path[index]) )
			index++;
		if ( path[index] == '/' )
			index++;
	}
	return true;
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Print the full filename of the current working directory.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -L, --logical   use PWD from environment, even if it contains symlinks\n");
	fprintf(fp, "  -P, --physical  avoid all symlinks\n");
	fprintf(fp, "      --help      display this help and exit\n");
	fprintf(fp, "      --version   output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool physical = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'L': physical = false; break;
			case 'P': physical = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--logical") )
			physical = false;
		else if ( !strcmp(arg, "--physical") )
			physical = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	// The get_current_dir_name function will use the PWD variable if it is
	// accurate, so we'll need to unset it if it is inappropriate to use it.
	const char* pwd_env;
	if ( (pwd_env = getenv("PWD")) )
	{
		if ( physical || !is_path_absolute(pwd_env) )
			unsetenv("PWD");
	}

	char* pwd = get_current_dir_name();
	if ( !pwd )
		error(1, errno, "get_current_dir_name");

	printf("%s\n", pwd);

	if ( ferror(stdout) || fflush(stdout) == EOF )
		error(1, errno, "stdout");

	return 0;
}
