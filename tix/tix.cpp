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

    tix.cpp
    Front end to the Tix package management system.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

typedef struct
{
	char* collection;
	char* tar;
	char* tixdb_path;
	char* tix_install;
	string_array_t coll_conf;
	string_array_t repo_list;
	string_array_t inst_list;
} params_t;

char* FindPackageInRepository(const char* repo, const char* pkg_name)
{
	char* repo_index_path = join_paths(repo, "repository.index");
	string_array_t repo_index = string_array_make();
	if ( !dictionary_append_file_path(&repo_index, repo_index_path) )
		error(1, errno, "bad repository: `%s'", repo_index_path);
	free(repo_index_path);
	const char* pkg_path_rel = dictionary_get(&repo_index, pkg_name);
	char* ret = pkg_path_rel ? join_paths(repo, pkg_path_rel) : NULL;
	string_array_reset(&repo_index);
	return ret;
}

char* FindPackage(string_array_t* repositories, const char* pkg_name)
{
	for ( size_t i = 0; i < repositories->length; i++ )
	{
		const char* repo = repositories->strings[i];
		char* ret = FindPackageInRepository(repo, pkg_name);
		if ( ret )
			return ret;
	}
	return NULL;
}

string_array_t GetPackageDependencies(params_t* params, const char* pkg_name)
{
	string_array_t ret = string_array_make();

	char* pkg_path = FindPackage(&params->repo_list, pkg_name);
	if ( !pkg_path )
		error(1, errno, "unable to locate package `%s'", pkg_name);

	const char* tixinfo_path = "tix/tixinfo";
	if ( !TarContainsFile(params->tar, pkg_path, tixinfo_path) )
		error(1, 0, "`%s' doesn't contain a `%s' file", pkg_path, tixinfo_path);

	string_array_t tixinfo = string_array_make();
	FILE* tixinfo_fp = TarOpenFile(params->tar, pkg_path, tixinfo_path);
	dictionary_append_file(&tixinfo, tixinfo_fp);
	fclose(tixinfo_fp);

	VerifyTixInformation(&tixinfo, pkg_path);

	const char* deps = dictionary_get(&tixinfo, "pkg.runtime-deps", "");
	string_array_append_token_string(&ret, deps);

	string_array_reset(&tixinfo);

	free(pkg_path);

	return ret;
}

void GetPackageRecursiveDependencies(params_t* params, string_array_t* sofar,
                                     const char* pkg_name)
{
	if ( string_array_contains(sofar, pkg_name) )
		return;

	// Avoid endless recursion by adding our package before the recursive call,
	// in case we need to satisfy cyclic dependencies.
	string_array_append(sofar, pkg_name);

	string_array_t pkg_deps = GetPackageDependencies(params, pkg_name);
	for ( size_t i = 0; i < pkg_deps.length; i++ )
		if ( !string_array_contains(sofar, pkg_deps.strings[i]) )
			GetPackageRecursiveDependencies(params, sofar, pkg_deps.strings[i]);
	string_array_reset(&pkg_deps);
}

void InstallPackageOfName(params_t* params, const char* pkg_name)
{
	char* pkg_path = FindPackage(&params->repo_list, pkg_name);
	if ( !pkg_path )
		error(1, errno, "unable to locate package `%s'", pkg_name);

	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			params->tix_install,
			"--collection", params->collection,
			"--tar", params->tar,
			"--", pkg_path,
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "`%s'", cmd_argv[0]);
	}

	free(pkg_path);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s PREFIX COMMAND [OPTION]...\n", argv0);
	fprintf(fp, "Front end to the Tix package management system.\n");
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
	params_t params;
	params.collection = strdup_null(getenv_def("TIX_COLLECTION", NULL));
	params.tar = strdup(getenv_def("TAR", "tar"));
	params.tix_install = strdup("tix-install");

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
		else if ( GET_OPTION_VARIABLE("--collection", &params.collection) ) { }
		else if ( GET_OPTION_VARIABLE("--tar", &params.tar) ) { }
		else if ( GET_OPTION_VARIABLE("--tix-install", &params.tix_install) ) { }
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

	ParseOptionalCommandLineCollectionPrefix(&params.collection, &argc, &argv);
	VerifyCommandLineCollection(&params.collection);

	params.tixdb_path = join_paths(params.collection, "tix");

	char* coll_conf_path = join_paths(params.tixdb_path, "collection.conf");
	params.coll_conf = string_array_make();
	if ( !dictionary_append_file_path(&params.coll_conf, coll_conf_path) )
		error(1, errno, "`%s'", coll_conf_path);
	VerifyTixCollectionConfiguration(&params.coll_conf, coll_conf_path);
	free(coll_conf_path);

	char* repo_list_path = join_paths(params.tixdb_path, "repository.list");
	params.repo_list = string_array_make();
	if ( !string_array_append_file_path(&params.repo_list, repo_list_path) )
		error(1, errno, "`%s'", repo_list_path);
	free(repo_list_path);

	char* inst_list_path = join_paths(params.tixdb_path, "installed.list");
	params.inst_list = string_array_make();
	if ( !string_array_append_file_path(&params.inst_list, inst_list_path) )
		error(1, errno, "`%s'", inst_list_path);
	free(inst_list_path);

	if ( argc == 1 )
	{
		error(0, 0, "error: no command specified.");
		exit(1);
	}

	const char* cmd = argv[1];
	if ( !strcmp(cmd, "install") )
	{
		if ( argc == 2 )
		{
			error(0, 0, "expected list of packages to install after `install'");
			exit(1);
		}

		string_array_t work = string_array_make();

		for ( int i = 2; i < argc; i++ )
		{
			const char* pkg_name = argv[i];
			if ( string_array_contains(&params.inst_list, pkg_name) )
			{
				printf("Package `%s' is already installed.\n", pkg_name);
				continue;
			}

			GetPackageRecursiveDependencies(&params, &work, pkg_name);
		}

		for ( size_t i = 0; i < work.length; i++ )
			InstallPackageOfName(&params, work.strings[i]);

		string_array_reset(&work);

		return 0;
	}
	else
	{
		fprintf(stderr, "%s: unknown command: `%s'\n", argv0, cmd);
		exit(1);
	}
}
