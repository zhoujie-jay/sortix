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
 * mv.c
 * Rename files and directories.
 */

#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* basename_dup(const char* path)
{
	char* copy = strdup(path);
	if ( !copy )
		return (char*) NULL;
	char* result = strdup(basename(copy));
	free(copy);
	if ( !result )
		return (char*) NULL;
	return result;
}

static const int FLAG_VERBOSE = 1 << 0;
static const int FLAG_ASK_OVERWRITE = 1 << 1;

bool mv(int src_dirfd, const char* src_rel, const char* src_path,
        int dst_dirfd, const char* dst_rel, const char* dst_path,
        int flags)
{
	if ( flags & FLAG_VERBOSE )
		printf("`%s' -> `%s'\n", src_path, dst_path);
	if ( flags & FLAG_ASK_OVERWRITE )
	{
		// TODO: Ask for confirmation if needed.
	}
	if ( renameat(src_dirfd, src_rel, dst_dirfd, dst_rel) < 0 )
	{
		error(0, errno, "renaming `%s' to `%s'", src_path, dst_path);
		return false;
	}
	return true;
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
	fprintf(fp, "Usage: %s [OPTION]... [-T] SOURCE DEST\n", argv0);
	fprintf(fp, "  or:  %s [OPTION]... SOURCE... DIRECTORY\n", argv0);
	fprintf(fp, "  or:  %s [OPTION]... -t DIRECTORY SOURCE...\n", argv0);
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	int flags = 0;
	bool flag_no_target_directory = false;
	const char* target_directory = NULL;

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
			case 'f': flags &= ~FLAG_ASK_OVERWRITE; break;
#if 0
			case 'i': flags |= FLAG_ASK_OVERWRITE; break;
			case 'n': flags |= FLAG_NO_CLOBBER; break;
#endif
			case 't':
				if ( *(arg + 1) )
					target_directory = arg + 1;
				else if ( i + 1 == argc )
				{
					error(0, 0, "option requires an argument -- '%c'", c);
					fprintf(stderr, "Try '%s --help' for more information\n", argv0);
					exit(1);
				}
				else
				{
					target_directory = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "t";
				break;
			case 'T': flag_no_target_directory = true; break;
#if 0
			case 'u': flags |= FLAG_UPDATE; break;
#endif
			case 'v': flags |= FLAG_VERBOSE; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--target-directory") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--target-directory 'requires an argument");
				fprintf(stderr, "Try '%s --help' for more information\n", argv0);
				exit(1);
			}
			target_directory = argv[++i];
			argv[i] = NULL;
		}
		else if ( !strncmp(arg, "--target-directory=", strlen("--target-directory=")) )
			target_directory = arg + strlen("--target-directory=");
		else if ( !strcmp(arg, "--no-target-directory") )
			flag_no_target_directory = true;
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

	if ( target_directory && flag_no_target_directory )
		error(1, 0, "cannot combine --target-directory (-t) and --no-target-directory (-T)");

	compact_arguments(&argc, &argv);

	if ( argc < 2 )
		error(1, 0, "missing file operand");

	if ( !target_directory && !flag_no_target_directory && argc == 3 )
	{
		const char* dst = argv[2];
		struct stat dst_st;
		if ( stat(dst, &dst_st) < 0 )
			flag_no_target_directory = true;
		else if ( !S_ISDIR(dst_st.st_mode) )
			flag_no_target_directory = true;
	}

	if ( flag_no_target_directory )
	{
		const char* src = argv[1];
		if ( argc < 3 )
			error(1, 0, "missing destination file operand after `%s'", src);
		const char* dst = argv[2];
		if ( 3 < argc )
			error(1, 0, "extra operand `%s'", argv[3]);
		return mv(AT_FDCWD, src, src,
		          AT_FDCWD, dst, dst,
		          flags) ? 0 : 1;
	}

	if ( !target_directory )
	{
		target_directory = argv[--argc];
		argv[argc] = NULL;
	}

	int dst_dirfd = open(target_directory, O_RDONLY | O_DIRECTORY);
	if ( dst_dirfd < 0 )
		error(1, errno, "`%s'", target_directory);

	if ( argc < 2 )
		error(1, 0, "missing file operand");

	const char* join = "/";
	if ( strlen(target_directory) &&
	     target_directory[strlen(target_directory)-1] == '/' )
		join = "";

	bool success = true;
	for ( int i = 1; i < argc; i++ )
	{
		const char* src = argv[i];
		int src_dirfd = AT_FDCWD;
		const char* src_rel = src;
		const char* src_path = src;
		char* dst_rel = basename_dup(src_path);
		if ( !dst_rel )
		{
			error(0, errno, "strdup");
			success = false;
			continue;
		}
		char* dst_path = NULL;
		asprintf(&dst_path, "%s%s%s", target_directory, join, dst_rel);
		if ( !dst_path )
		{
			error(0, errno, "asprintf");
			free(dst_rel);
			success = false;
			continue;
		}
		if ( !mv(src_dirfd, src_rel, src_path,
		         dst_dirfd, dst_rel, dst_path,
		         flags) )
			success = false;
		free(dst_path);
		free(dst_rel);
	}

	close(dst_dirfd);

	return success ? 0 : 1;
}
