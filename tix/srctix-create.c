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
 * srctix-create.c
 * Converts an archived port tix into an archived source tix.
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

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... PORT-TIX\n", argv0);
	fprintf(fp, "Converts an archived port tix into an archived source tix.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

bool is_file_name(const char* path)
{
	return !(strchr(path, '/') || !strcmp(path, ".") || !strcmp(path, ".."));
}

int main(int argc, char* argv[])
{
	char* output_directory = strdup(".");
	char* output = NULL;
	char* tmp = strdup(getenv_def("TMPDIR", "/tmp"));

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
		else if ( GET_OPTION_VARIABLE("--output-directory", &output_directory) ) { }
		else if ( GET_OPTION_VARIABLE("--output", &output) ) { }
		else if ( GET_OPTION_VARIABLE("--tmp", &tmp) ) { }
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
		error(1, 0, "No archived port tix specified");

	if ( 3 <= argc )
		error(1, 0, "extra operand");

	const char* porttix_path = argv[1];

	char* tmp_in_root = print_string("%s/srctixin.XXXXXX", tmp);
	if ( !mkdtemp(tmp_in_root) )
		error(1, errno, "mkdtemp: `%s'", tmp_in_root);
	on_exit(cleanup_file_or_directory, tmp_in_root);

	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			"tar",
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

	char* tmp_out_root = print_string("%s/srctixout.XXXXXX", tmp);
	if ( !mkdtemp(tmp_out_root) )
		error(1, errno, "mkdtemp: `%s'", tmp_out_root);
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
			if ( mkdir_p(srctix_path, 0755) != 0 )
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
					"tar",
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
					"tix-rmpatch",
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
					"patch",
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
					"tix-execpatch",
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
			"tar",
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
