/*
 * Copyright (c) 2015 Alexandros Alexandrou.
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
 * touch.c
 * Change file timestamp.
 */

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
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

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILE]\n", argv0);
	fprintf(fp, "Update access and/or modification time of FILE(s).\n");
	fprintf(fp, "  -a           Change only access time.\n");
	fprintf(fp, "  -c           Don't create a specified file if it doesn't exist.\n");
	fprintf(fp, "  -m           Change only modification time.\n");
	fprintf(fp, "  -r ref_file  Refer to access and modifiaction times of ref_file.\n");
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool opt_a = false;
	bool opt_c = false;
	bool opt_m = false;
	const char* opt_r = NULL;

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
			while ( (c = *++arg) ) switch (c)
			{
			case 'a': opt_a = true; break;
			case 'c': opt_c = true; break;
			// TODO: -d
			case 'm': opt_m = true; break;
			case 'r':
				if ( !*(opt_r = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						fprintf(stderr, "option requires an argument -- 'r'\n");
						fprintf(stderr, "Try '%s --help' for more information.\n", argv0);
						exit(125);
					}
					opt_r = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "r";
				break;
			// TODO: -t
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				fprintf(stderr, "Try '%s --help' for more information.\n", argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option -- '%s'\n", argv0, arg);
			fprintf(stderr, "Try '%s --help' for more information.\n", argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( argc == 1 )
		errx(1, "missing file operand");

	if ( !opt_a && !opt_m )
	{
		opt_a = true;
		opt_m = true;
	}

	struct timespec times[2] =
	{
		timespec_make(0, UTIME_NOW),
		timespec_make(0, UTIME_NOW),
	};

	if ( opt_r )
	{
		struct stat st;
		if ( stat(opt_r, &st) < 0 )
			err(1, "%s", opt_r);
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
	}

	if ( !opt_a )
		times[0] = timespec_make(0, UTIME_OMIT);
	if ( !opt_m )
		times[1] = timespec_make(0, UTIME_OMIT);

	for ( int i = 1; i < argc; i++ )
	{
		const char* path = argv[i];
		if ( utimens(path, times) < 0 )
		{
			if ( errno == ENOENT )
			{
				if ( opt_c )
					continue;
				int fd;
				if ( (fd = open(path, O_CREAT | O_RDWR, 0666)) < 0 )
					err(1, "%s", opt_r);
				if ( futimens(fd, times) < 0 )
					err(1, "%s", opt_r);
				close(fd);
			}
			else
				err(1, "%s", path);
		}
	}

	return 0;
}
