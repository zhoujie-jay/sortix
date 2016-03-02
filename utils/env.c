/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * env.c
 * Set the environment for command invocation.
 */

#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void clean_environment(void)
{
	while ( environ[0] )
	{
		size_t equal_pos = strcspn(environ[0], "=");
		if ( environ[0][equal_pos] != '=' )
			error(125, 0, "Bad Environment");
		char* key = strndup(environ[0], equal_pos);
		if ( !key )
			error(125, errno, "strndup");
		if ( unsetenv(key) != 0 )
			error(125, errno, "cannot unset `%s'", key);
		free(key);
	}
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [-] [NAME=VALUE]... [COMMAND [ARG]...]\n", argv0);
	fprintf(fp, "Set each NAME to VALUE in the environment and run COMMAND.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -i, --ignore-environment  start with an empty environment\n");
	fprintf(fp, "  -0, --null           end each output line with 0 byte rather than newline\n");
	fprintf(fp, "  -u, --unset=NAME     remove variable from the environment\n");
	fprintf(fp, "      --help           display this help and exit\n");
	fprintf(fp, "      --version        output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "A mere - implies -i.  If no COMMAND, print the resulting environment.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	bool ignore_environment = false;
	bool null_terminate = false;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			break; // Intentionally not continue.
		if ( arg[0] == '-' && !arg[1] )
		{
			ignore_environment = true;
			break;
		}
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'i': ignore_environment = true; break;
			case 'u':
				if ( !arg[1] )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'u'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					const char* key = argv[i+1];
					argv[++i] = NULL;
					if ( unsetenv(key) != 0 )
						error(125, errno, "cannot unset `%s'", key);
				}
				else
				{
					const char* key = arg + 1;
					if ( unsetenv(key) != 0 )
						error(125, errno, "cannot unset `%s'", key);
				}
				arg = "u";
				break;
			case '0': null_terminate = true; break;
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
		else if ( !strcmp(arg, "--ignore-environment") )
			ignore_environment = true;
		else if ( !strcmp(arg, "--null") )
			null_terminate = true;
		else if ( !strncmp(arg, "--unset=", strlen("--unset=")) )
		{
			const char* key = arg + strlen("--unset=");
			if ( unsetenv(key) != 0 )
				error(125, errno, "cannot unset `%s'", key);
		}
		else if ( !strcmp(arg, "--unset") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--unset' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			const char* key = argv[i+1];
			argv[++i] = NULL;
			if ( unsetenv(key) != 0 )
				error(125, errno, "cannot unset `%s'", key);
		}
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	if ( ignore_environment )
		clean_environment();

	compact_arguments(&argc, &argv);

	for ( int i = 1; i < argc; i++ )
	{
		char* arg = argv[i];
		size_t equal_pos = strcspn(arg, "=");
		if ( arg[equal_pos] != '=' )
			break;
		argv[i] = NULL;
		arg[equal_pos] = '\0';
		const char* key = arg;
		const char* value = arg + equal_pos + 1;
		if ( setenv(key, value, 1) != 0 )
			error(125, errno, "setenv(\"%s\", \"%s\")", key, value);
	}

	compact_arguments(&argc, &argv);

	if ( argc == 1 )
	{
		for ( size_t i = 0; environ[i]; i++ )
		{
			fputs(environ[i], stdout);
			fputc(null_terminate ? '\0' : '\n', stdout);
		}
		return 0;
	}

	if ( null_terminate )
	{
		error(0, 0, "cannot specify --null (-0) with command");
		fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
		exit(125);
	}

	execvp(argv[1], argv + 1);
	error(errno == ENOENT ? 127 : 126, errno, "%s", argv[1]);
}
