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
 * ln.c
 * Create a hard or symbolic link.
 */

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	fprintf(fp, "Usage: %s [OPTION]... TARGET LINK_NAME\n", argv0);
	fprintf(fp, "Create a hard or symbolic link.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool force = false;
	bool symbolic = false;
	bool no_target_directory = false;
	bool verbose = false;
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
			case 'f': force = true; break;
			case 's': symbolic = true; break;
			case 'T': no_target_directory = true; break;
			case 'v': verbose = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--force") )
			force = true;
		else if ( !strcmp(arg, "--symbolic") )
			symbolic = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( argc != 3 )
		error(1, 0, "%s operand", argc < 3 ? "missing" : "extra");

	const char* oldname = argv[1];
	const char* newname = argv[2];

	bool done = false;
again:
	if ( force )
		unlink(newname);

	struct stat st;
	int ret = (symbolic ? symlink : link)(oldname, newname);
	if ( ret == 0  )
	{
		if ( verbose )
			printf("`%s' => `%s'\n", newname, oldname);
	}
	else if ( !done && errno == EEXIST && !no_target_directory &&
	          lstat(newname, &st) == 0 && S_ISDIR(st.st_mode) )
	{
		char* oldnamecopy = strdup(oldname);
		const char* name = basename(oldnamecopy);
		char* newnewname;
		asprintf(&newnewname, "%s/%s", newname, name);
		free(oldnamecopy);
		newname = newnewname;
		done = true;
		goto again;
	}
	else
		error(0, errno, "`%s' => `%s'", newname, oldname);
	return ret ? 1 : 0;
}
