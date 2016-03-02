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
 * porttix-create.c
 * Creates a port tix by generating patches using source code and tarballs.
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

int redirect(const char* path, int flags, mode_t mode)
{
	int fd = open(path, flags, mode);
	if ( fd < 0 )
		return -1;
	dup2(fd, 1);
	close(fd);
	return 0;
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... --tarball=TARBALL --normalized=NORMALIZED SOURCE-TIX\n", argv0);
	fprintf(fp, "Creates a port tix by generating patches using source code and tarballs.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}
int main(int argc, char* argv[])
{
	char* input_normalized_path = NULL;
	char* input_tarball_path = NULL;
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
		else if ( GET_OPTION_VARIABLE("--normalized", &input_normalized_path) ) { }
		else if ( GET_OPTION_VARIABLE("--output-directory", &output_directory) ) { }
		else if ( GET_OPTION_VARIABLE("--output", &output) ) { }
		else if ( GET_OPTION_VARIABLE("--tarball", &input_tarball_path) ) { }
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
	{
		fprintf(stderr, "%s: no source tix specified\n", argv0);
		help(stderr, argv0);
		exit(1);
	}

	if ( 3 <= argc )
	{
		fprintf(stderr, "%s: unexpected extra operand `%s'\n", argv0, argv[2]);
		help(stderr, argv0);
		exit(1);
	}

	const char* input_srctix_path = argv[1];

	if ( !IsDirectory(input_srctix_path) )
		error(1, errno, "`%s'", input_srctix_path);

	char* tixbuildinfo_path = print_string("%s/tixbuildinfo", input_srctix_path);

	string_array_t package_info = string_array_make();
	if ( !dictionary_append_file_path(&package_info, tixbuildinfo_path) )
	{
		if ( errno == ENOENT )
			fprintf(stderr, "%s: `%s' doesn't appear to be a source tix:\n",
			                argv0, input_srctix_path);
		error(1, errno, "`%s'", tixbuildinfo_path);
	}

	const char* package_name = strdup(dictionary_get(&package_info, "pkg.name"));

	if ( !output )
		output = print_string("%s/%s.porttix.tar.xz", output_directory, package_name);

	char* tmp_root = print_string("%s/porttix.XXXXXX", tmp);
	if ( !mkdtemp(tmp_root) )
		error(1, errno, "mkdtemp: `%s'", tmp_root);

	on_exit(cleanup_file_or_directory, tmp_root);

	const char* tarball_basename = non_modify_basename(input_tarball_path);

	char* rel_srctix_path = print_string("%s.srctix", package_name);
	char* rel_normalized_path = print_string("%s.normalized", package_name);

	char* porttix_path = print_string("%s/%s", tmp_root, package_name);
	if ( mkdir_p(porttix_path, 0755) != 0 )
		error(1, errno, "mkdir: `%s'", porttix_path);

	char* srctix_path = print_string("%s/%s", tmp_root, rel_srctix_path);
	if ( mkdir_p(srctix_path, 0755) != 0 )
		error(1, errno, "mkdir: `%s'", srctix_path);

	char* normalized_path = print_string("%s/%s", tmp_root, rel_normalized_path);
	if ( mkdir_p(normalized_path, 0755) != 0 )
		error(1, errno, "mkdir: `%s'", normalized_path);

	// Create the porttixinfo file.
	char* porttixinfo_path = join_paths(porttix_path, "porttixinfo");
	FILE* porttixinfo_fp = fopen(porttixinfo_path, "w");
	if ( !porttixinfo_fp )
		error(1, errno, "`%s'", porttixinfo_path);
	fprintf(porttixinfo_fp, "package_name %s\n", package_name);

	// Copy the input source tix to the temporary root.
	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			"cp",
			"-HRT",
			"--preserve=timestamps,links",
			"--",
			input_srctix_path,
			srctix_path,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}

	// If no tarball exists, then package up the source directory!
	if ( !input_tarball_path )
	{
		input_tarball_path = print_string("%s/%s.tar.xz", tmp_root, package_name);
		if ( fork_and_wait_or_death() )
		{
			char* work_dir = dirname(strdup(srctix_path));
			char* subdir_name = dirname(strdup(srctix_path));
			if ( chdir(work_dir) != 0 )
				error(1, errno, "chdir: `%s'", work_dir);
			const char* cmd_argv[] =
			{
				"tar",
				"--create",
				"--xz",
				"--directory", input_normalized_path,
				"--file", input_tarball_path,
				"--",
				subdir_name,
				NULL,
			};
			execvp(cmd_argv[0], (char* const*) cmd_argv);
			error(127, errno, "%s", cmd_argv[0]);
		}
	}

	// Copy the normalized directory (if one exists) to the temporary root.
	if ( input_normalized_path )
	{
		if ( fork_and_wait_or_death() )
		{
			const char* cmd_argv[] =
			{
				"cp",
				"-HRT",
				"--preserve=timestamps,links",
				"--",
				input_normalized_path,
				normalized_path,
				NULL,
			};
			execvp(cmd_argv[0], (char* const*) cmd_argv);
			error(127, errno, "%s", cmd_argv[0]);
		}
	}

	// There is no input normalized directory, so just extract the tarball here.
	else
	{
		if ( fork_and_wait_or_death() )
		{
			const char* cmd_argv[] =
			{
				"tar",
				"--extract",
				"--directory", normalized_path,
				"--file", input_tarball_path,
				"--strip-components=1",
				NULL,
			};
			execvp(cmd_argv[0], (char* const*) cmd_argv);
			error(127, errno, "%s", cmd_argv[0]);
		}
	}

	// Copy the tarball into the port tix.
	char* porttix_tarball_path = print_string("%s/%s", porttix_path, tarball_basename);
	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			"cp",
			"--",
			input_tarball_path,
			porttix_tarball_path,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	fprintf(porttixinfo_fp, "tar_extract %s\n", tarball_basename);

	// Create the normalization patch.
	int normalized_fd = open(normalized_path, O_RDONLY | O_DIRECTORY);
	if ( normalized_fd < 0 )
		error(1, errno, "`%s'", normalized_path);

	char* patch_normalize_path = join_paths(porttix_path, "patch.normalize");
	FILE* patch_normalize_fp = fopen(patch_normalize_path, "w");
	if ( !patch_normalize_fp )
		error(1, errno, "`%s'", patch_normalize_path);

	int pipes[2];
	if ( pipe(pipes) )
		error(1, errno, "pipe");
	pid_t tar_pid = fork_or_death();
	if ( !tar_pid )
	{
		dup2(pipes[1], 1);
		close(pipes[1]);
		close(pipes[0]);
		const char* cmd_argv[] =
		{
			"tar",
			"--list",
			"--file", porttix_tarball_path,
			"--strip-components=1",
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	close(pipes[1]);
	FILE* tar_fp = fdopen(pipes[0], "r");

	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	while ( 0 < (line_len = getline(&line, &line_size, tar_fp)) )
	{
		if ( line_len && line[line_len-1] == '\n' )
			line[--line_len] = '\0';
		const char* path = line;
		while ( *path && *path != '/' )
			path++;
		if ( *path == '/' )
			path++;
		if ( !*path )
			continue;
		struct stat st;
		if ( fstatat(normalized_fd, path, &st, 0) != 0 && errno == ENOENT )
		{
			fprintf(patch_normalize_fp, "rm -rf -- '");
			for ( size_t i = 0; path[i]; i++ )
				if ( path[i] == '\'' )
					fprintf(patch_normalize_fp, "'\\''");
				else
					fputc(path[i], patch_normalize_fp);
			fprintf(patch_normalize_fp, "'\n");
		}
	}
	free(line);

	fclose(tar_fp);
	int tar_exit_status;
	waitpid(tar_pid, &tar_exit_status, 0);
	if ( !WIFEXITED(tar_exit_status) || WEXITSTATUS(tar_exit_status) != 0 )
	{
		error(1, 0, "Unable to list contents of `%s'.", porttix_tarball_path);
		exit(WEXITSTATUS(tar_exit_status));
	}

	fclose(patch_normalize_fp);
	free(patch_normalize_path);
	fprintf(porttixinfo_fp, "apply_normalize patch.normalize\n");

	close(normalized_fd);

	// Create the patch between the source tix and the normalized tree.
	char* patch_path = join_paths(porttix_path, "patch.patch");
	if ( fork_and_wait_or_death_def(false) )
	{
		close(1);
		if ( open(patch_path, O_WRONLY | O_CREAT | O_TRUNC, 0644) != 1 )
			error(1, errno, "`%s'", patch_path);
		if ( chdir(tmp_root) != 0 )
			error(1, errno, "chdir(`%s')", tmp_root);
		const char* cmd_argv[] =
		{
			"diff",
			"--no-dereference",
			"-Naur",
			"--",
			rel_normalized_path,
			rel_srctix_path,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	free(patch_path);
	fprintf(porttixinfo_fp, "apply_patch patch.patch\n");

	// Created the execpatch between the source tix and the normalized tree.
	char* patch_exec_path = join_paths(porttix_path, "patch.execpatch");
	if ( fork_and_wait_or_death_def(false) )
	{
		if ( redirect(patch_exec_path, O_WRONLY | O_CREAT | O_TRUNC, 0644) != 0 )
			error(1, errno, "`%s'", patch_exec_path);
		if ( chdir(tmp_root) != 0 )
			error(1, errno, "chdir(`%s')", tmp_root);
		const char* cmd_argv[] =
		{
			"tix-execdiff",
			"--",
			rel_normalized_path,
			rel_srctix_path,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	free(patch_exec_path);
	fprintf(porttixinfo_fp, "apply_execpatch patch.execpatch\n");

	// Close the porttixinfo file.
	fclose(porttixinfo_fp);
	free(porttixinfo_path);

	// Package up the output archived port tix.
	if ( fork_and_wait_or_death() )
	{
		const char* cmd_argv[] =
		{
			"tar",
			"--create",
			"--xz",
			"--directory", tmp_root,
			"--file", output,
			"--",
			package_name,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}

	return 0;
}
