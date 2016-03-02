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
 * cat.c
 * Concatenate and print files to the standard output.
 */

#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool cat_fd(int fd, const char* path)
{
	struct stat st;
	if ( fstat(fd, &st) == 0 )
	{
		if ( S_ISDIR(st.st_mode) )
			return error(0, EISDIR, "`%s'", path), false;
	}

	const size_t BUFFER_SIZE = 16 * 1024;
	uint8_t buffer[BUFFER_SIZE];

	ssize_t buffer_used;
	while ( 0 < (buffer_used = read(fd, buffer, BUFFER_SIZE)) )
	{
		size_t so_far = 0;
		while ( so_far < (size_t) buffer_used )
		{
			ssize_t amount = write(1, buffer + so_far, buffer_used - so_far);
			if ( amount <= 0 )
				return error(0, errno, "`%s'", "<stdout>"), false;
			so_far += amount;
		}
	}

	if ( buffer_used < 0 )
		return error(0, errno, "`%s'", path), false;

	return true;
}

static bool cat_path(const char* path)
{
	if ( !strcmp("-", path) )
		return cat_fd(0, "<stdin>");
	int fd = open(path, O_RDONLY);
	if ( fd < 0 )
	{
		error(0, errno, "`%s'", path);
		return false;
	}
	bool result = cat_fd(fd, path);
	close(fd);
	return result;
}

static bool cat_arguments(int argc, char* argv[])
{
	if ( argc <= 1 )
		return cat_path("-");
	bool success = true;
	for ( int i = 1; i < argc; i++ )
		if ( !cat_path(argv[i]) )
			success = false;
	return success;
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
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
	fprintf(fp, "Concatenate FILE(s), or standard input, to standard output.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -u                       (ignored)\n");
	fprintf(fp, "      --help               display this help and exit\n");
	fprintf(fp, "      --version            output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "With no FILE, or when FILE is -, read standard input.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
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
			case 'u': /* Ignored, POSIX compatibility. */ break;
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	return cat_arguments(argc, argv) ? 0 : 1;
}
