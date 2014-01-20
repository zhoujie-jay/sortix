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

    srctix-create.cpp
    Converts an archived port tix into an archived source tix.

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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... PORT-TIX\n", argv0);
	fprintf(fp, "Converts an archived port tix into an archived source tix.\n");
}

void version(FILE* fp, const char* argv0)
{
	help(fp, argv0);
}

bool is_file_name(const char* path)
{
	return !(strchr(path, '/') || !strcmp(path, ".") || !strcmp(path, ".."));
}

int main(int argc, char* argv[])
{
	char* output_directory = strdup(".");
	char* output = NULL;
	char* patch = strdup(getenv_def("PATCH", "patch"));
	char* tar = strdup(getenv_def("TAR", "tar"));
	char* tix_execpatch = strdup(getenv_def("TIX_EXECPATCH", "tix-execpatch"));
	char* tix_rmpatch = strdup(getenv_def("TIX_RMPATCH", "tix-rmpatch"));
	char* tmp = strdup(getenv_def("TMP", "/tmp"));

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
		else if ( GET_OPTION_VARIABLE("--output-directory", &output_directory) ) { }
		else if ( GET_OPTION_VARIABLE("--output", &output) ) { }
		else if ( GET_OPTION_VARIABLE("--patch", &patch) ) { }
		else if ( GET_OPTION_VARIABLE("--tar", &tar) ) { }
		else if ( GET_OPTION_VARIABLE("--tix-execpatch", &tix_execpatch) ) { }
		else if ( GET_OPTION_VARIABLE("--tix-rmpatch", &tix_rmpatch) ) { }
		else if ( GET_OPTION_VARIABLE("--tmp", &tmp) ) { }
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

	if ( argc <= 1 )
	{
		fprintf(stderr, "%s: no archived port tix specified\n", argv0);
		help(stderr, argv0);
		exit(1);
	}

	if ( 3 <= argc )
	{
		fprintf(stderr, "%s: unexpected extra operand `%s'\n", argv0, argv[2]);
		help(stderr, argv0);
		exit(1);
	}

	const char* porttix_path = argv[1];

	char* tmp_in_root = print_string("%s/tmppid.%ju.in", tmp, (uintmax_t) getpid());
	if ( mkdir_p(tmp_in_root, 0777) != 0 )
		error(1, errno, "mkdir: `%s'", tmp_in_root);
	on_exit(cleanup_file_or_directory, tmp_in_root);

	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			tar,
			"--extract",
			"--directory", tmp_in_root,
			"--file", porttix_path,
			"--strip-components=1",
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}

	char* porttixinfo_path = join_paths(tmp_in_root, "porttixinfo");
	FILE* porttixinfo_fp = fopen(porttixinfo_path, "r");
	if ( !porttixinfo_fp )
	{
		if ( errno == ENOENT )
			error(0, 0, "`%s' doesn't appear to be an archived port tix",
			            porttix_path);
		error(1, errno, "`%s'", porttixinfo_path);
	}

	char* tmp_out_root = print_string("%s/tmppid.%ju.out", tmp, (uintmax_t) getpid());
	if ( mkdir_p(tmp_out_root, 0777) != 0 )
		error(1, errno, "mkdir: `%s'", tmp_out_root);
	on_exit(cleanup_file_or_directory, tmp_out_root);

	char* package_name = NULL;
	char* srctix_path = NULL;

	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	while ( 0 < (line_len = getline(&line, &line_size, porttixinfo_fp)) )
	{
		if ( line_len && line[line_len-1] == '\n' )
			line[--line_len] = '\0';
		char* first_space = strchr(line, ' ');
		if ( !first_space )
			error(1, errno, "`%s`: malformed line `%s'",
			      porttixinfo_path, line);
		*first_space = '\0';
		const char* function = line;
		const char* parameter = first_space + 1;

		if ( !strcmp(function, "package_name") )
		{
			if ( package_name )
				error(1, errno, "`%s`: unexpected additional package name `%s'",
				      porttixinfo_path, parameter);
			if ( !is_file_name(parameter) )
				error(1, errno, "`%s`: malformed package name `%s'",
				      porttixinfo_path, parameter);
			package_name = strdup(parameter);
			srctix_path = join_paths(tmp_out_root, package_name);
			if ( mkdir_p(srctix_path, 0777) != 0 )
				error(1, errno, "mkdir: `%s'", srctix_path);
		}
		else if ( !package_name )
			error(1, errno, "`%s`: expected package name before `%s'",
			      porttixinfo_path, function);
		else if ( !strcmp(function, "tar_extract") )
		{
			if ( !is_file_name(parameter) )
				error(1, errno, "`%s`: malformed tarball filename `%s'",
				      porttixinfo_path, parameter);
			char* tarball_path = join_paths(tmp_in_root, parameter);
			if ( fork_and_wait_or_death() )
			{
				const char* cmd_argv[] =
				{
					tar,
					"--extract",
					"--directory", srctix_path,
					"--file", tarball_path,
					"--strip-components=1",
					NULL,
				};
				execvp(cmd_argv[0], (char* const*) cmd_argv);
				error(127, errno, "%s", cmd_argv[0]);
			}
			free(tarball_path);
		}
		else if ( !strcmp(function, "apply_normalize") )
		{
			if ( !is_file_name(parameter) )
				error(1, errno, "`%s`: malformed normalize filename `%s'",
				      porttixinfo_path, parameter);
			char* rmpatch_path = join_paths(tmp_in_root, parameter);
			if ( fork_and_wait_or_death() )
			{
				const char* cmd_argv[] =
				{
					tix_rmpatch,
					"--directory", srctix_path,
					"--",
					rmpatch_path,
					NULL,
				};
				execvp(cmd_argv[0], (char* const*) cmd_argv);
				error(127, errno, "%s", cmd_argv[0]);
			}
			free(rmpatch_path);
		}
		else if ( !strcmp(function, "apply_patch") )
		{
			if ( !is_file_name(parameter) )
				error(1, errno, "`%s`: malformed patch filename `%s'",
				      porttixinfo_path, parameter);
			char* patch_path = join_paths(tmp_in_root, parameter);
			if ( fork_and_wait_or_death() )
			{
				const char* cmd_argv[] =
				{
					patch,
					"--strip=1",
					"--silent",
					"--directory", srctix_path,
					"--input", patch_path,
					NULL,
				};
				execvp(cmd_argv[0], (char* const*) cmd_argv);
				error(127, errno, "%s", cmd_argv[0]);
			}
			free(patch_path);
		}
		else if ( !strcmp(function, "apply_execpatch") )
		{
			if ( !is_file_name(parameter) )
				error(1, errno, "`%s`: malformed execpatch filename `%s'",
				      porttixinfo_path, parameter);
			char* execpatch_path = join_paths(tmp_in_root, parameter);
			if ( fork_and_wait_or_death() )
			{
				const char* cmd_argv[] =
				{
					tix_execpatch,
					"--directory", srctix_path,
					"--",
					execpatch_path,
					NULL,
				};
				execvp(cmd_argv[0], (char* const*) cmd_argv);
				error(127, errno, "%s", cmd_argv[0]);
			}
			free(execpatch_path);
		}
		else
			error(1, errno, "`%s`: unsupported function `%s'",
			      porttixinfo_path, function);
	}
	free(line);

	fclose(porttixinfo_fp);
	free(porttixinfo_path);

	if ( !output )
		output = print_string("%s/%s.srctix.tar.xz", output_directory, package_name);

	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			tar,
			"--create",
			"--xz",
			"--directory", tmp_out_root,
			"--file", output,
			"--",
			package_name,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}

	free(srctix_path);
	free(package_name);

	return 0;
}
