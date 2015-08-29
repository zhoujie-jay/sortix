/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    tix-collection.cpp
    Administer and configure a tix collection.

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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [PREFIX] [OPTION]... COMMAND\n", argv0);
	fprintf(fp, "Administer and configure a tix collection.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	char* collection = NULL;
	char* platform = NULL;
	char* prefix = NULL;
	// TODO: After releasing Sortix 1.0, keep the --disable-multiarch option
	//       supported (but ignored), delete all uses of --disable-multiarch,
	//       delete the --enable-multiarch option, delete the use_multiarch=true
	//       case code. Simplify all of this code, remove the tixdb abstraction.
	// TODO: After releasing Sortix 1.1, delete the --disable-multiarch option
	//       compatibility.
#if defined(__sortix__)
	bool use_multiarch = false;
#else
	bool use_multiarch = true;
#endif
	char* generation_string = strdup(DEFAULT_GENERATION);

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
			while ( char c = *++arg ) switch ( c )
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
		else if ( GET_OPTION_VARIABLE("--collection", &collection) ) { }
		else if ( GET_OPTION_VARIABLE("--platform", &platform) ) { }
		else if ( GET_OPTION_VARIABLE("--prefix", &prefix) ) { }
		else if ( GET_OPTION_VARIABLE("--generation", &generation_string) ) { }
		else if ( !strcmp(arg, "--enable-multiarch") )
			use_multiarch = true;
		else if ( !strcmp(arg, "--disable-multiarch") )
			use_multiarch = false;
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

	ParseOptionalCommandLineCollectionPrefix(&collection, &argc, &argv);
	VerifyCommandLineCollection(&collection);

	int generation = atoi(generation_string);
	free(generation_string);
	(void) generation;

	if ( !prefix )
		prefix = strdup(collection);

	if ( argc == 1 )
	{
		error(0, 0, "error: no command specified.");
		exit(1);
	}

	const char* cmd = argv[1];
	if ( !strcmp(cmd, "create") )
	{
		if ( !platform && !(platform = GetBuildTriplet()) )
			error(1, errno, "unable to determine platform, use --platform");

		char* tix_path = join_paths(collection, "tix");
		if ( mkdir_p(tix_path, 0755) != 0 )
			error(1, errno, "mkdir: `%s'", tix_path);

		char* tixdb_path;
		if ( use_multiarch )
		{
			tixdb_path = join_paths(tix_path, platform);
			if ( mkdir_p(tixdb_path, 0755) != 0 )
				error(1, errno, "mkdir: `%s'", tixdb_path);
		}
		else
		{
			tixdb_path = strdup(tix_path);
		}

		char* collection_conf_path = join_paths(tixdb_path, "collection.conf");
		FILE* conf_fp = fopen(collection_conf_path, "wx");
		if ( !conf_fp && errno == EEXIST )
			error(1, 0, "error: `%s' already exists, a tix collection is "
			            "already installed at `%s'.", collection_conf_path,
		                collection);
		fprintf(conf_fp, "tix.version=1\n");
		fprintf(conf_fp, "tix.class=collection\n");
		fprintf(conf_fp, "collection.prefix=%s\n", !strcmp(prefix, "/") ? "" :
		                                           prefix);
		fprintf(conf_fp, "collection.platform=%s\n", platform);
		fclose(conf_fp);
		free(collection_conf_path);

		const char* repo_list_path = join_paths(tixdb_path, "repository.list");
		FILE* repo_list_fp = fopen(repo_list_path, "w");
		if ( !repo_list_fp )
			error(1, errno, "`%s'", repo_list_path);
		fclose(repo_list_fp);

		const char* inst_list_path = join_paths(tixdb_path, "installed.list");
		FILE* inst_list_fp = fopen(inst_list_path, "w");
		if ( !inst_list_fp )
			error(1, errno, "`%s'", inst_list_path);
		fclose(inst_list_fp);

		return 0;
	}
	else
	{
		fprintf(stderr, "%s: unknown command: `%s'\n", argv0, cmd);
		exit(1);
	}

	return 0;
}
