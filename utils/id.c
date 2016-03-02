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
 * id.c
 * Return user identity.
 */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <grp.h>
#include <locale.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char* name_of_group_by_gid(gid_t gid)
{
	errno = 0;
	struct group* grp = getgrgid(gid);
	if ( !grp && errno == 0 )
		errx(1, "gid %" PRIuGID " has no group entry", gid);
	if ( !grp )
		err(1, "gid %" PRIuGID "", gid);
	return grp->gr_name;
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
	fprintf(fp, "Usage: %s [OPTION]... [USERNAME]\n", argv0);
	fprintf(fp, "Print user and group information for the specified USERNAME,\n");
	fprintf(fp, "or (when USERNAME omitted) for the current user.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -g, --group    print only the effective group ID\n");
	fprintf(fp, "  -G, --groups   print all group IDs\n");
	fprintf(fp, "  -n, --name     print a name instead of a number, for -ugG\n");
	fprintf(fp, "  -r, --real     print the real ID instead of the effective ID, with -ugG\n");
	fprintf(fp, "  -u, --user     print only the effective user ID\n");
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool groups_only = false;
	bool group_only = false;
	bool user_only = false;
	bool names = false;
	bool real = false;

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
			case 'G': groups_only = true; break;
			case 'g': group_only = true; break;
			case 'n': names = true; break;
			case 'r': real = true; break;
			case 'u': user_only = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--group") )
			group_only = true;
		else if ( !strcmp(arg, "--groups") )
			groups_only = true;
		else if ( !strcmp(arg, "--name") )
			names = true;
		else if ( !strcmp(arg, "--real") )
			real = true;
		else if ( !strcmp(arg, "--user") )
			user_only = true;
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

	if ( 1 < groups_only + group_only + user_only )
		errx(1, "options -Ggu are mutually exclusive");

	(void) real; // Sortix only has real user ids.

	struct passwd* pwd;
	if ( 2 < argc )
		errx(1, "extra operand '%s'", argv[2]);
	else if ( argc == 2 )
	{
		errno = 0;
		pwd = getpwnam(argv[1]);
		if ( !pwd && errno == 0 )
			errx(1, "%s: no such user", argv[1]);
		if ( !pwd )
			err(1, "%s", argv[1]);
	}
	else
	{
		errno = 0;
		pwd = getpwuid(getuid());
		if ( !pwd && errno == 0 )
			errx(1, "current user has no user entry");
		if ( !pwd )
			err(1, "current user");
	}

	if ( groups_only )
	{
		// TODO: Sortix doesn't have getgroups at the moment.
		if ( names )
			printf("%s", name_of_group_by_gid(pwd->pw_gid));
		else
			printf("%" __PRIuGID, pwd->pw_gid);
		setgrent();
		struct group* grp;
		while ( (errno = 0, grp = getgrent()) )
		{
			if ( grp->gr_gid == pwd->pw_gid )
				continue;
			bool member = false;
			for ( size_t i = 0; !member && grp->gr_mem[i]; i++ )
				member = !strcmp(pwd->pw_name, grp->gr_mem[i]);
			if ( !member )
				continue;
			if ( names )
				printf(" %s", grp->gr_name);
			else
				printf(" %" __PRIuGID "", grp->gr_gid);
		}
		if ( errno != 0 )
			err(1, "getgrent");
		endgrent();
		printf("\n");
	}
	else if ( group_only )
	{
		// If current user, should this display the getgid() group instead?
		if ( names )
			printf("%s\n", name_of_group_by_gid(pwd->pw_gid));
		else
			printf("%" __PRIuGID "\n", pwd->pw_gid);
	}
	else if ( user_only )
	{
		if ( names )
			printf("%s\n", pwd->pw_name);
		else
			printf("%" __PRIuUID "\n", pwd->pw_uid);
	}
	else
	{
		if ( names || real )
			errx(1, "cannot print only names or real IDs in default format");
		name_of_group_by_gid(pwd->pw_gid); // Early error.
		printf("uid=%" PRIuUID "(%s) ", pwd->pw_uid, pwd->pw_name);
		printf("gid=%" PRIiGID "(%s) ", pwd->pw_gid, name_of_group_by_gid(pwd->pw_gid));
		printf("groups=");
		const char* prefix = "";
		setgrent();
		struct group* grp;
		while ( (errno = 0, grp = getgrent()) )
		{
			bool member = false;
			for ( size_t i = 0; !member && grp->gr_mem[i]; i++ )
				member = !strcmp(pwd->pw_name, grp->gr_mem[i]);
			if ( !member )
				continue;
			printf("%s%" __PRIuGID "(%s)", prefix, grp->gr_gid, grp->gr_name);
			prefix = ",";
		}
		if ( errno != 0 )
			err(1, "getgrent");
		endgrent();
		printf("\n");
	}

	if ( ferror(stdout) || fflush(stdout) == EOF )
		return 1;
	return 0;
}
