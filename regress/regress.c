/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * regress.c
 * Automatically invokes system tests.
 */

#include <sys/wait.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static const int VERBOSITY_SILENT = 0;
static const int VERBOSITY_NO_OUTPUT = 1;
static const int VERBOSITY_QUIET = 2;
static const int VERBOSITY_NORMAL = 3;
static const int VERBOSITY_VERBOSE = 4;

bool is_usable_terminal(int fd)
{
	struct wincurpos wcp;
	struct winsize ws;
	return isatty(fd) &&
	       tcgetwinsize(fd, &ws) == 0 &&
	       tcgetwincurpos(fd, &wcp) == 0 &&
	       10 <= ws.ws_col;
}

void tenth_last_column(void)
{
	struct wincurpos wcp;
	struct winsize ws;
	fflush(stdout);
	tcgetwinsize(1, &ws);
	tcgetwincurpos(1, &wcp);
	if ( wcp.wcp_col - ws.ws_col < 10 )
		printf("\n");
	printf("\e[%zuG", ws.ws_col - 10);
	fflush(stdout);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Automatically invoke system test cases.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -b, --buffered        buffer up test output in a pipe\n");
	fprintf(fp, "  -q  --quiet           output only failed tests\n");
	fprintf(fp, "  -s  --silent          don't output test results\n");
	fprintf(fp, "      --testdir=DIR     use test programs in DIR\n");
	fprintf(fp, "  -u, --unbuffered      send test output immediately to the terminal\n");
	fprintf(fp, "  -v  --verbose         verbose output\n");
	fprintf(fp, "      --help            display this help and exit\n");
	fprintf(fp, "      --version         output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
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

bool get_option_variable(const char* option, char** varptr,
                         const char* arg, int argc, char** argv, int* ip,
                         const char* argv0)
{
	size_t option_len = strlen(option);
	if ( strncmp(option, arg, option_len) != 0 )
		return false;
	if ( arg[option_len] == '=' )
	{
		*varptr = strdup(arg + option_len + 1);
		return true;
	}
	if ( arg[option_len] != '\0' )
		return false;
	if ( *ip + 1 == argc )
	{
		fprintf(stderr, "%s: expected operand after `%s'\n", argv0, option);
		exit(1);
	}
	*varptr = strdup(argv[++*ip]), argv[*ip] = NULL;
	return true;
}

static int no_dot_files(const struct dirent* entry)
{
	return entry->d_name[0] != '.' ? 1 : 0;
}

#define GET_OPTION_VARIABLE(str, varptr) \
        get_option_variable(str, varptr, arg, argc, argv, &i, argv0)

#define PIPE_BUFFER_SIZE 65536
static unsigned char pipe_buffer[PIPE_BUFFER_SIZE];

int main(int argc, char* argv[])
{
	bool buffered = true;
#ifdef TESTDIR
	char* testdir_path = strdup(TESTDIR);
#else
	char* testdir_path = NULL;
#endif
	int verbosity = VERBOSITY_NORMAL;

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
			case 'b': buffered = true; break;
			case 'q': verbosity--; break;
			case 's': verbosity = VERBOSITY_SILENT; break;
			case 'u': buffered = false; break;
			case 'v': verbosity++; break;
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
		else if ( !strcmp(arg, "--buffered") )
			buffered = true;
		else if ( !strcmp(arg, "--quiet") )
			verbosity--;
		else if ( !strcmp(arg, "--silent") )
			verbosity = VERBOSITY_SILENT;
		else if ( GET_OPTION_VARIABLE("--testdir", &testdir_path) ) { }
		else if ( !strcmp(arg, "--unbuffered") )
			buffered = false;
		else if ( !strcmp(arg, "--verbose") )
			verbosity++;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( !testdir_path )
		error(1, 0, "No test directory was specified and no default available");

	struct dirent** entries;
	int num_entries = scandir(testdir_path, &entries, no_dot_files, alphasort);
	if ( num_entries < 0 )
		error(2, errno, "scandir: `%s'", testdir_path);

	int exit_status = 0;
	bool use_terminal = is_usable_terminal(1);

	for ( int i = 0; i < num_entries; i++ )
	{
		const struct dirent* entry = entries[i];

		const char* test_name = entry->d_name;
		char* test_path;
		asprintf(&test_path, "%s/%s", testdir_path, test_name);

		bool partial_begun_line = false;
		if ( VERBOSITY_NORMAL <= verbosity )
		{
			if ( use_terminal )
			{
				printf("%s ", test_path);
				tenth_last_column();
				printf("[      ]");
				if ( buffered )
					partial_begun_line = true;
				else
					printf("\n");
				fflush(stdout);
			}
			else if ( !buffered )
			{
				printf("%s [      ]\n", test_path);
				fflush(stdout);
			}
		}

		int pipe_fds[2];
		if ( buffered && pipe(pipe_fds) < 0 )
			error(1, errno, "pipe");

		pid_t child_pid = fork();
		if ( child_pid < 0 )
			error(1, errno, "fork");

		if ( !child_pid )
		{
			int dev_null_fd = open("/dev/null", O_RDWR);

			if ( 0 < dev_null_fd )
				dup2(dev_null_fd, 0);

			if ( verbosity <= VERBOSITY_NO_OUTPUT )
			{
				dup2(dev_null_fd, 1);
				dup2(dev_null_fd, 2);
			}
			else if ( buffered )
			{
				dup2(pipe_fds[1], 1);
				dup2(pipe_fds[1], 2);
			}

			if ( buffered )
			{
				close(pipe_fds[0]);
				close(pipe_fds[1]);
			}

			if ( 0 < dev_null_fd )
				close(dev_null_fd);

			char* child_argv[] = { test_path, NULL };
			execv(child_argv[0], child_argv);
			error(127, errno, "`%s'", test_path);
		}

		size_t bytes_read = 0;
		if ( VERBOSITY_NO_OUTPUT < verbosity && buffered )
		{
			close(pipe_fds[1]);

			while ( bytes_read < PIPE_BUFFER_SIZE )
			{
				ssize_t amount = read(pipe_fds[0],
				                      pipe_buffer + bytes_read,
				                      PIPE_BUFFER_SIZE - bytes_read);
				if ( !amount )
					break;
				if ( amount < 0 )
					break;
				bytes_read += amount;
			}

			close(pipe_fds[0]);
		}

		int child_exit_code;
		if ( waitpid(child_pid, &child_exit_code, 0) < 0 )
			error(1, errno, "waitpid(%ji)", (intmax_t) child_pid);

		bool success = WIFEXITED(child_exit_code) &&
		               WEXITSTATUS(child_exit_code) == 0;

		if ( !success )
			exit_status = 1;

		if ( (VERBOSITY_NORMAL <= verbosity && success) ||
		     (VERBOSITY_NO_OUTPUT <= verbosity && !success ) )
		{
			if ( use_terminal )
			{
				if ( !partial_begun_line )
					printf("%s ", test_path);
				tenth_last_column();
				printf("%s\n", success ?
				               "[\e[32mPASSED\e[m]" :
				               "[\e[31mFAILED\e[m]");
			}
			else
			{
				if ( success )
					printf("%s [PASSED]\n", test_path);
				else
					printf("%s [FAILED]\n", test_path);
			}

			if ( VERBOSITY_NO_OUTPUT < verbosity && buffered )
				fwrite(pipe_buffer, 1, bytes_read, stdout);

			fflush(stdout);
		}
	}

	for ( int i = 0; i < num_entries; i++ )
		free(entries[i]);
	free(entries);

	return exit_status;
}
