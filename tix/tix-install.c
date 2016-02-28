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

    tix-install.c
    Install a tix into a tix collection.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <error.h>
#include <errno.h>
#include <dirent.h>
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

void TipTixCollection(const char* prefix)
{
	error(0, 0, "error: `%s' isn't a tix collection, use \"tix-collection %s "
	            "create\" before " "installing packages.", prefix, prefix);
}

void VerifyTixCollection(const char* prefix)
{
	if ( !IsDirectory(prefix) )
	{
		if ( errno == ENOENT )
			TipTixCollection(prefix);
		error(1, errno, "error: tix collection unavailable: `%s'", prefix);
	}
}

void VerifyTixDirectory(const char* prefix, const char* tix_dir)
{
	if ( !IsDirectory(tix_dir) )
	{
		if ( errno == ENOENT )
			TipTixCollection(prefix);
		error(1, errno, "error: tix database unavailable: `%s'", tix_dir);
	}
}

void VerifyTixDatabase(const char* prefix,
                       const char* tixdb_path)
{
	if ( !IsDirectory(tixdb_path) )
		error(1, errno, "error: tix database unavailable: `%s'", tixdb_path);
	char* info_path = join_paths(tixdb_path, "collection.conf");
	if ( !IsFile(info_path) )
	{
		if ( errno == ENOENT )
			TipTixCollection(prefix);
		error(1, errno, "error: tix collection information unavailable: `%s'",
		                info_path);
	}
	char* installed_list_path = join_paths(tixdb_path, "installed.list");
	FILE* installed_list_fp = fopen(installed_list_path, "a");
	if ( !installed_list_fp )
	{
		error(0, errno, "error: unable to open `%s' for writing",
		                installed_list_path);
		error(1, 0, "error: `%s': do you have sufficient permissions to "
		            "administer this tix collection?", prefix);
	}
	fclose(installed_list_fp);
	free(installed_list_path);
	free(info_path);
}

bool IsPackageInstalled(const char* tixdb_path, const char* package)
{
	char* installed_list_path = join_paths(tixdb_path, "installed.list");
	FILE* installed_list_fp = fopen(installed_list_path, "r");
	if ( !installed_list_fp )
		error(1, errno, "`%s'", installed_list_path);

	bool ret = false;
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	while ( 0 < (line_len = getline(&line, &line_size, installed_list_fp)) )
	{
		if ( line_len && line[line_len-1] == '\n' )
			line[--line_len] = '\0';
		if ( !strcmp(line, package) )
		{
			ret = true;
			break;
		}
	}
	free(line);

	fclose(installed_list_fp);
	free(installed_list_path);
	return ret;
}

void MarkPackageAsInstalled(const char* tixdb_path, const char* package)
{
	char* installed_list_path = join_paths(tixdb_path, "installed.list");
	FILE* installed_list_fp = fopen(installed_list_path, "a");
	if ( !installed_list_fp )
		error(1, errno, "`%s'", installed_list_path);

	fprintf(installed_list_fp, "%s\n", package);
	fflush(installed_list_fp);

	if ( ferror(installed_list_fp) || fclose(installed_list_fp) == EOF )
		error(1, errno, "`%s'", installed_list_path);
	free(installed_list_path);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [--collection=PREFIX] PACKAGE...\n", argv0);
	fprintf(fp, "Install a tix into a tix collection.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

static char* collection = NULL;
static bool reinstall = false;
static char* tix_directory_path = NULL;

void InstallPackage(const char* tix_path);

int main(int argc, char* argv[])
{
	collection = strdup("/");

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
		else if ( GET_OPTION_VARIABLE("--collection", &collection) ) { }
		else if ( !strcmp(arg, "--reinstall") )
			reinstall = true;
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

	if ( argc <= 1 )
	{
		fprintf(stderr, "%s: no package specified\n", argv0);
		exit(1);
	}

	if ( !*collection )
	{
		free(collection);
		collection = strdup("/");
	}

	VerifyTixCollection(collection);

	tix_directory_path = join_paths(collection, "tix");

	VerifyTixDirectory(collection, tix_directory_path);

	for ( int i = 1; i < argc; i++ )
		InstallPackage(argv[i]);

	free(tix_directory_path);

	return 0;
}

static int strcmp_indirect(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return strcmp(a, b);
}

void InstallPackage(const char* tix_path)
{
	if ( !IsFile(tix_path) )
		error(1, errno, "`%s'", tix_path);

	const char* tixinfo_path = "tix/tixinfo";
	if ( !TarContainsFile(tix_path, tixinfo_path) )
		error(1, 0, "`%s' doesn't contain a `%s' file", tix_path, tixinfo_path);

	string_array_t tixinfo = string_array_make();
	FILE* tixinfo_fp = TarOpenFile(tix_path, tixinfo_path);
	dictionary_append_file(&tixinfo, tixinfo_fp);
	fclose(tixinfo_fp);

	const char* package_name = dictionary_get(&tixinfo, "pkg.name");
	assert(package_name);

	const char* package_prefix = dictionary_get(&tixinfo, "pkg.prefix");
	assert(package_prefix || !package_prefix);

	const char* package_platform = dictionary_get(&tixinfo, "tix.platform");
	assert(package_platform || !package_platform);

	// TODO: After releasing Sortix 1.0, delete this compatibility.
	//       Then move this code to main, from here...
	char* tixdb_path = join_paths(tix_directory_path, package_platform);
	if ( !IsDirectory(tixdb_path ) )
	{
		free(tixdb_path);
		tixdb_path = strdup(tix_directory_path);
	}

	VerifyTixDatabase(collection, tixdb_path);

	char* coll_conf_path = join_paths(tixdb_path, "collection.conf");
	string_array_t coll_conf = string_array_make();
	if ( !dictionary_append_file_path(&coll_conf, coll_conf_path) )
		error(1, errno, "`%s'", coll_conf_path);
	VerifyTixCollectionConfiguration(&coll_conf, coll_conf_path);
	free(coll_conf_path);
	// ... to here. (But see allocation cleanup below).

	const char* coll_generation = dictionary_get(&coll_conf, "collection.generation");
	// TODO: After releasing Sortix 1.0, remove this compatibility.
	if ( !coll_generation )
		coll_generation = "1";
	assert(coll_generation);
	int generation = atoi(coll_generation);
	(void) generation;
	const char* coll_prefix = dictionary_get(&coll_conf, "collection.prefix");
	assert(coll_prefix);
	const char* coll_platform = dictionary_get(&coll_conf, "collection.platform");
	assert(coll_platform);

	bool already_installed = IsPackageInstalled(tixdb_path, package_name);
	if ( already_installed && !reinstall )
		error(1, 0, "error: package `%s' is already installed. Use --reinstall "
		            "to force reinstallation.", package_name);

	if ( package_prefix && strcmp(coll_prefix, package_prefix) != 0 )
	{
		error(0, errno, "error: `%s' is compiled with the prefix `%s', but the "
		                "destination collection has the prefix `%s'.", tix_path,
		                package_prefix, coll_prefix);
		error(1, errno, "you need to recompile the package with "
		                "--prefix=\"%s\".", coll_prefix);
	}

	if ( package_platform && strcmp(coll_platform, package_platform) != 0 )
	{
		error(0, errno, "error: `%s' is compiled with the platform `%s', but "
		                "the destination collection has the platform `%s'.",
		                tix_path, package_platform, coll_platform);
		error(1, errno, "you need to recompile the package with "
		                "--host=%s\".", coll_platform);
	}

	printf("Installing `%s' into `%s'...\n", package_name, collection);
	char* data_and_prefix = package_prefix && package_prefix[0] ?
	                        print_string("data%s", package_prefix) :
	                        strdup("data");

	// TODO: After releasing Sortix 1.0, do this unconditionally.
	if ( 2 <= generation )
	{
		char* tixinfo_path = print_string("%s/tixinfo/%s", tixdb_path, package_name);
		int tixinfo_fd = open(tixinfo_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if ( tixinfo_fd < 0 )
			error(1, errno, "%s", tixinfo_path);
		TarExtractFileToFD(tix_path, "tix/tixinfo", tixinfo_fd);
		close(tixinfo_fd);

		FILE* index_fp = TarOpenIndex(tix_path);
		string_array_t files = string_array_make();
		string_array_append_file(&files, index_fp);
		qsort(files.strings, files.length, sizeof(char*), strcmp_indirect);
		char* manifest_path = print_string("%s/manifest/%s", tixdb_path, package_name);
		FILE* manifest_fp = fopen(manifest_path, "w");
		if ( !manifest_fp )
			error(1, errno, "%s", manifest_path);
		for ( size_t i = 0; i < files.length; i++ )
		{
			char* str = files.strings[i];
			if ( strncmp(str, "data", strlen("data")) != 0 )
				continue;
			str += strlen("data");
			if ( str[0] && str[0] != '/' )
				continue;
			size_t len = strlen(str);
			while ( 2 <= len && str[len-1] == '/' )
				str[--len] = '\0';
			if ( fprintf(manifest_fp, "%s\n", str) < 0 )
				error(1, errno, "%s", manifest_path);
		}
		if ( ferror(manifest_fp) || fflush(manifest_fp) == EOF )
			error(1, errno, "%s", manifest_path);
		fclose(manifest_fp);
		string_array_reset(&files);
		fclose(index_fp);
	}

	if ( fork_and_wait_or_death() )
	{
		size_t num_strips = count_tar_components(data_and_prefix);
		const char* cmd_argv[] =
		{
			"tar",
			print_string("--strip-components=%zu", num_strips),
			"-C", collection,
			"--extract",
			"--file", tix_path,
			"--keep-directory-symlink",
			data_and_prefix,
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	free(data_and_prefix);

	if ( !already_installed )
		MarkPackageAsInstalled(tixdb_path, package_name);

	string_array_reset(&tixinfo);

	// TODO: After releasing Sortix 1.0, and done the above, move this to main.
	string_array_reset(&coll_conf);
	free(tixdb_path);
}
