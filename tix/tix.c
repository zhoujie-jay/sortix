/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * tix.c
 * Front end to the Tix package management system.
 */

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
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

typedef struct
{
	char* collection;
	char* tixdb_path;
	string_array_t coll_conf;
	string_array_t repo_list;
	string_array_t inst_list;
	bool reinstall;
} params_t;

char* FindPackageInRepository(params_t* params,
                              const char* repo,
                              const char* pkg_name)
{
	char* repo_location;
	if ( repo[0] == '/' )
		repo_location = strdup(repo);
	else
		repo_location = join_paths(params->tixdb_path, repo);
	char* tix_name = print_string("%s.tix.tar.xz", pkg_name);
	char* tix_path = join_paths(repo_location, tix_name);
	free(repo_location);
	free(tix_name);
	if ( !IsFile(tix_path) )
	{
		free(tix_path);
		return NULL;
	}
	return tix_path;
}

char* FindPackage(params_t* params,
                  string_array_t* repositories,
                  const char* pkg_name)
{
	for ( size_t i = 0; i < repositories->length; i++ )
	{
		const char* repo = repositories->strings[i];
		char* ret = FindPackageInRepository(params, repo, pkg_name);
		if ( ret )
			return ret;
	}
	return NULL;
}

string_array_t GetPackageDependencies(params_t* params, const char* pkg_name)
{
	string_array_t ret = string_array_make();

	char* pkg_path = FindPackage(params, &params->repo_list, pkg_name);
	if ( !pkg_path )
		error(1, errno, "unable to locate package `%s'", pkg_name);

	const char* tixinfo_path = "tix/tixinfo";
	if ( !TarContainsFile(pkg_path, tixinfo_path) )
		error(1, 0, "`%s' doesn't contain a `%s' file", pkg_path, tixinfo_path);

	string_array_t tixinfo = string_array_make();
	FILE* tixinfo_fp = TarOpenFile(pkg_path, tixinfo_path);
	dictionary_append_file(&tixinfo, tixinfo_fp);
	fclose(tixinfo_fp);

	VerifyTixInformation(&tixinfo, pkg_path);

	const char* deps = dictionary_get_def(&tixinfo, "pkg.runtime-deps", "");
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
	char* pkg_path = FindPackage(params, &params->repo_list, pkg_name);
	if ( !pkg_path )
		error(1, errno, "unable to locate package `%s'", pkg_name);

	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			"tix-install",
			"--collection", params->collection,
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
	fprintf(fp, "Usage: %s [PREFIX] COMMAND ...\n", argv0);
	fprintf(fp, "Front end to the Tix package management system.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Commands:\n");
	fprintf(fp, "  install [--reinstall] PACKAGE...\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	params_t params;
	memset(&params, 0, sizeof(params));
	params.collection = NULL;

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
		else if ( GET_OPTION_VARIABLE("--collection", &params.collection) ) { }
		else if ( !strcmp(arg, "--reinstall") )
			params.reinstall = true;
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
		error(1, 0, "error: no command specified.");

	const char* cmd = argv[1];
	if ( !strcmp(cmd, "install") )
	{
		if ( argc == 2 )
			error(1, 0, "expected list of packages to install after `install'");

		string_array_t work = string_array_make();

		for ( int i = 2; i < argc; i++ )
		{
			const char* pkg_name = argv[i];
			if ( string_array_contains(&params.inst_list, pkg_name) )
			{
				if ( params.reinstall )
				{
					string_array_append(&work, pkg_name);
					continue;
				}
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
