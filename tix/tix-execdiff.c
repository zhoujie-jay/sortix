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
 * tix-execdiff.c
 * Reports which files have had the executable bit changed between two trees.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

DIR* fdopendupdir(int fd)
{
	int newfd = dup(fd);
	if ( newfd < 0 )
		return NULL;
	DIR* result = fdopendir(newfd);
	if ( !result )
		return close(newfd), (DIR*) NULL;
	return result;
}

int ftestexecutableat(int dirfd, const char* path)
{
	struct stat st;
	if ( fstatat(dirfd, path, &st, AT_SYMLINK_NOFOLLOW) != 0 )
		return -1;
	if ( !S_ISREG(st.st_mode) )
		return errno = EISDIR, -1;
	if ( faccessat(dirfd, path, X_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW) == 0 )
		return 1;
	else if ( errno == EACCES )
		return 0;
	else
		return -1;
}

void execdiff(int tree_a, const char* tree_a_path,
              int tree_b, const char* tree_b_path,
              const char* relpath)
{
	DIR* dir_b = fdopendupdir(tree_b);
	if ( !dir_b )
		error(1, errno, "fdopendupdir(`%s`)", tree_b_path);
	struct dirent* entry;
	while ( (entry = readdir(dir_b)) )
	{
		if ( !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") )
			continue;

		char* subrelpath = join_paths(relpath, entry->d_name);

		int diropenflags = O_RDONLY | O_DIRECTORY | O_NOFOLLOW;
		int subtree_b = openat(tree_b, entry->d_name, diropenflags);
		if ( 0 <= subtree_b )
		{
			char* subtree_b_path = join_paths(tree_b_path, entry->d_name);
			int subtree_a = openat(tree_a, entry->d_name, diropenflags);
			if ( subtree_a < 0 )
			{
				if ( !(errno == ENOTDIR || errno == ELOOP || errno == ENOENT) )
					error(1, errno, "`%s/%s`", tree_b_path, entry->d_name);
				execdiff(-1, NULL, subtree_b, subtree_b_path, subrelpath);
				free(subtree_b_path);
				close(subtree_b);
				free(subrelpath);
				continue;
			}
			char* subtree_a_path = join_paths(tree_a_path, entry->d_name);
			execdiff(subtree_a, subtree_a_path, subtree_b, subtree_b_path,
			         subrelpath);
			free(subtree_a_path);
			close(subtree_a);
			free(subtree_b_path);
			close(subtree_b);
			free(subrelpath);
			continue;
		}
		else if ( !(errno == ENOTDIR || errno == ELOOP) )
			error(1, errno, "`%s/%s`", tree_b_path, entry->d_name);

		int a_executableness = ftestexecutableat(tree_a, entry->d_name);
		int b_executableness = ftestexecutableat(tree_b, entry->d_name);

		if ( (a_executableness == 1) && (b_executableness == 0) )
		{
			printf("chmod -x -- '");
			for ( size_t i = 0; subrelpath[i]; i++ )
				if ( subrelpath[i] == '\'' )
					printf("'\\''");
				else
					putchar(subrelpath[i]);
			printf("'\n");
		}

		if ( (a_executableness != 1) && (b_executableness == 1) )
		{
			printf("chmod +x -- '");
			for ( size_t i = 0; subrelpath[i]; i++ )
				if ( subrelpath[i] == '\'' )
					printf("'\\''");
				else
					putchar(subrelpath[i]);
			printf("'\n");
		}

		free(subrelpath);
		continue;
	}
	closedir(dir_b);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... TREE-A TREE-B\n", argv0);
	fprintf(fp, "Reports which files have had the executable bit changed between two trees.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	for ( int i = 0; i < argc; i++ )
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

	if ( argc == 1 )
	{
		help(stdout, argv0);
		exit(0);
	}

	compact_arguments(&argc, &argv);

	if ( argc < 3 )
		error(1, 0, "You need to specify two directories");

	if ( 3 < argc )
		error(1, 0, "extra operand");

	const char* tree_a_path = argv[1];
	int tree_a = open(tree_a_path, O_RDONLY | O_DIRECTORY);
	if ( tree_a < 0 )
		error(1, errno, "`%s'", tree_a_path);

	const char* tree_b_path = argv[2];
	int tree_b = open(tree_b_path, O_RDONLY | O_DIRECTORY);
	if ( tree_b < 0 )
		error(1, errno, "`%s'", tree_b_path);

	execdiff(tree_a, tree_a_path, tree_b, tree_b_path, ".");

	return 0;
}
