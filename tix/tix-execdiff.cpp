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

    tix-execdiff.cpp
    Reports which files have had the executable bit changed between two trees.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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
	while ( struct dirent* entry = readdir(dir_b) )
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

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... TREE-A TREE-B\n", argv0);
	fprintf(fp, "Reports which files have had the executable bit changed between two trees.\n");
}

void version(FILE* fp, const char* argv0)
{
	help(fp, argv0);
}

int main(int argc, char* argv[])
{
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
			default:
				fprintf(stderr, "%s: unknown option -- `%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") ) { help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { version(stdout, argv0); exit(0); }
		else
		{
			fprintf(stderr, "%s: unknown option: `%s'\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	if ( argc == 1 )
	{
		help(stdout, argv0);
		exit(0);
	}

	CompactArguments(&argc, &argv);

	if ( argc < 3 )
	{
		fprintf(stderr, "%s: you need to specify two directories\n", argv0);
		help(stderr, argv0);
		exit(1);
	}

	if ( 3 < argc )
	{
		fprintf(stderr, "%s: unexpected extra operand `%s'\n", argv0, argv[2]);
		help(stderr, argv0);
		exit(1);
	}

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
