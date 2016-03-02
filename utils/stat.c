/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * stat.c
 * Display file status.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <err.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char file_type_character(mode_t mode)
{
	if ( S_ISBLK(mode) )
		return 'b';
	if ( S_ISCHR(mode) )
		return 'c';
	if ( S_ISDIR(mode) )
		return 'd';
	if ( S_ISFIFO(mode) )
		return 'p';
	if ( S_ISLNK(mode) )
		return 'l';
	if ( S_ISREG(mode) )
		return '-';
	if ( S_ISSOCK(mode) )
		return 's';
	return '?';
}

static void render_perms(char perms[11], mode_t mode)
{
	perms[0] = file_type_character(mode);
	const char flagnames[] = { 'x', 'w', 'r' };
	for ( size_t i = 0; i < 9; i++ )
	{
		perms[9-i] = mode & 1 << i ? flagnames[i % 3] : '-';
	}
	if ( mode & S_ISUID )
		perms[3] = mode & 0100 ? 's' : 'S';
	if ( mode & S_ISGID )
		perms[6] = mode & 0010 ? 's' : 'S';
	if ( mode & S_ISVTX )
		perms[9] = mode & 0001 ? 't' : 'T';
	perms[10] = 0;
}

static void display_status(struct stat* st, const char* path)
{
	printf("File: `%s'", path);
	if ( S_ISLNK(st->st_mode) && (uintmax_t) st->st_size < (uintmax_t) SIZE_MAX )
	{
		size_t target_size = (size_t) st->st_size;
		char* target = (char*) malloc(target_size + 1);
		ssize_t target_read;
		if ( target && 0 <= (target_read = readlink(path, target, target_size)) )
		{
			target[target_read] = '\0';
			printf(" -> `%s'", target);
		}
		free(target);
	}
	printf("\n");
	printf("Size: %" PRIiOFF "\n", st->st_size);
	char perms[11];
	render_perms(perms, st->st_mode);
	printf("Mode: %" PRIoMODE "/%s", st->st_mode, perms);
	printf("\n");
	printf("Device: 0x%" PRIXDEV "\n", st->st_dev);
	printf("Inode: %" PRIuINO "\n", st->st_ino);
	printf("Links: %" PRIuNLINK "\n", st->st_nlink);
	printf("UID: %" PRIuUID "", st->st_uid);
	struct passwd* pwd = getpwuid(st->st_uid);
	if ( pwd )
		printf("/%s", pwd->pw_name);
	printf("\n");
	printf("GID: %" PRIuGID "", st->st_gid);
	struct group* grp = getgrgid(st->st_gid);
	if ( grp )
		printf("/%s", grp->gr_name);
	printf("\n");
	struct tm tm;
	localtime_r(&st->st_atim.tv_sec, &tm);
	printf("Access: %i-%02i-%02i %02i:%02i:%02i.%09li\n",
	       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec, st->st_atim.tv_nsec);
	localtime_r(&st->st_mtim.tv_sec, &tm);
	printf("Modify: %i-%02i-%02i %02i:%02i:%02i.%09li\n",
	       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec, st->st_mtim.tv_nsec);
	localtime_r(&st->st_ctim.tv_sec, &tm);
	printf("Change: %i-%02i-%02i %02i:%02i:%02i.%09li\n",
	       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	       tm.tm_hour, tm.tm_min, tm.tm_sec, st->st_ctim.tv_nsec);
	printf("Blocksize: %" PRIiBLKSIZE "\n", st->st_blksize);
	printf("Blocks: %" PRIiBLKCNT "\n", st->st_blocks);
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
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "List processes.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool dereference = false;

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
			case 'L': dereference = true; break;
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
		else if ( !strcmp(arg, "--dereference") )
			dereference = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	int result = 0;

	if ( argc <= 1 )
	{
		struct stat st;
		if ( fstat(0, &st) < 0 )
			err(1, "fstat: %s", "<stdin>");
		display_status(&st, "<stdin>");
	}
	else for ( int i = 1; i < argc; i++ )
	{
		struct stat st;
		if ( (dereference ? stat : lstat)(argv[i], &st) < 0 )
		{
			warn("fstat: %s", argv[i]);
			result = 1;
			continue;
		}
		display_status(&st, argv[i]);
	}

	return ferror(stdout) || fflush(stdout) == EOF ? 1 : result;
}
