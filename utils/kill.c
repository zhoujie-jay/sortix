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
 * kill.c
 * Terminate or signal processes.
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* signames[128] =
{
	[0] = NULL,
	[SIGHUP] = "HUP",
	[SIGINT] = "INT",
	[SIGQUIT] = "QUIT",
	[SIGILL] = "ILL",
	[SIGTRAP] = "TRAP",
	[SIGABRT] = "ABRT",
	[SIGBUS] = "BUS",
	[SIGFPE] = "FPE",
	[SIGKILL] = "KILL",
	[SIGUSR1] = "USR1",
	[SIGSEGV] = "SEGV",
	[SIGUSR2] = "USR2",
	[SIGPIPE] = "PIPE",
	[SIGALRM] = "ALRM",
	[SIGTERM] = "TERM",
	[SIGSYS] = "SYS",
	[SIGCHLD] = "CHLD",
	[SIGCONT] = "CONT",
	[SIGSTOP] = "STOP",
	[SIGTSTP] = "TSTP",
	[SIGTTIN] = "TTIN",
	[SIGTTOU] = "TTOU",
	[SIGURG] = "URG",
	[SIGXCPU] = "XCPU",
	[SIGXFSZ] = "XFSZ",
	[SIGVTALRM] = "VTALRM",
	[SIGPWR] = "PWR",
	[SIGWINCH] = "WINCH",
};

__attribute__((constructor))
static void signames_init(void)
{
	static char sigrtnames[SIGRTMAX - SIGRTMIN + 1][10];
	int rtnum = SIGRTMAX - SIGRTMIN + 1;
	for ( int i = 0; i < rtnum; i++ )
	{
		char* name = sigrtnames[i];
		size_t name_size = sizeof(sigrtnames[i]);
		if ( i == SIGRTMIN )
			name = (char*) "RTMIN";
		else if ( i == SIGRTMIN )
			name = (char*) "RTMAX";
		else if ( i < rtnum / 2 )
			snprintf(name, name_size, "RTMIN+%i", i);
		else
			snprintf(name, name_size, "RTMAX-%i", SIGRTMAX - i);
		signames[SIGRTMIN + i] = name;
	}
}

static bool parse_signum(int* signumptr, const char* str)
{
	if ( !isdigit((unsigned char) str[0]) )
		return false;
	char* end;
	errno = 0;
	long l = strtol((char*) str, &end, 10);
	if ( end[0] )
		return false;
	if ( errno == ERANGE || l != (int) l)
		return false;
	if ( l < 0 || 128 <= l )
		return false;
	*signumptr = (int) l;
	return true;
}

static pid_t parse_pid(pid_t* pidptr, const char* str)
{
	if ( !isdigit((unsigned char) str[0]) )
		return false;
	char* end;
	errno = 0;
	intmax_t imax = strtoimax((char*) str, &end, 10);
	if ( end[0] )
		return false;
	if ( errno == ERANGE || imax != (pid_t) imax )
		return false;
	*pidptr = (pid_t) imax;
	return true;
}

static const char* exit_status_to_signame(int status)
{
	if ( 0 <= status && status <= 127 )
		return signames[status];
	if ( 128 <= status && status <= 255 )
		return signames[status - 128];
	return NULL;
}

static int signame_to_signum(const char* name)
{
	int signum;
	if ( parse_signum(&signum, name) )
		return signum;
	for ( int i = 0; i < 128; i++ )
		if ( signames[i] && !strcmp(signames[i], name) )
			return i;
	return -1;
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
	fprintf(fp, "Usage: %s [OPTION]... PID...\n", argv0);
	fprintf(fp, "Terminate or signal processes by pid.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -l, --list                list all signal names, or convert one to a name\n");
	fprintf(fp, "  -s, --signal=SIGNAL       specify signal to be sent\n");
	fprintf(fp, "      --help                display this help and exit\n");
	fprintf(fp, "      --version             output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool list = false;
	const char* signal = NULL;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( isupper((unsigned char) arg[1]) ||
		     isdigit((unsigned char) arg[1]) )
			signal = arg + 1;
		else if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'l': list = true; break;
			case 's':
				if ( !*(signal = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 's'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					signal = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "s";
				break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			list = true;
		else if ( !strncmp(arg, "--signal=", strlen("--signal=")) )
			signal = arg + strlen("--signal=");
		else if ( !strcmp(arg, "--signal") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--signal' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			signal = argv[i+1];
			argv[++i] = NULL;
		}
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

	if ( signal && list )
		error(1, 0, "options --signal and --list are mutually incompatible");

	if ( list && argc == 1 )
	{
		const char* prefix = "";
		for ( int i = 0; i < 128; i++ )
		{
			if ( !signames[i] )
				continue;
			if ( SIGRTMIN < i && i < SIGRTMAX )
				continue;
			printf("%s%s", prefix, signames[i]);
			prefix = " ";
		}
		printf("\n");
		return ferror(stdout) || fflush(stdout) == EOF ? 1 : 0;
	}
	else if ( list )
	{
		int result = 0;
		for ( int i = 1; i < argc; i++ )
		{
			int exit_status = atoi(argv[i]);
			const char* name = exit_status_to_signame(exit_status);
			if ( name )
				printf("%s\n", name);
			else
				result = 1;
		}
		return ferror(stdout) || fflush(stdout) == EOF ? 1 : result;
	}
	else
	{
		if ( argc == 1)
		{
			fprintf(stderr, "%s: missing operand\n", argv0);
			help(stderr, argv0);
			exit(1);
		}
		if ( !signal )
			signal = "TERM";
		int signum = signame_to_signum(signal);
		if ( signum < 0 )
			error(1, 0, "invalid signal: %s", signal);
		int result = 0;
		for ( int i = 1; i < argc; i++ )
		{
			pid_t pid = 0;
			if ( !parse_pid(&pid, argv[i]) )
				error(1, 0, "invalid process identifier: %s", argv[i]);
			if ( kill(pid, signum) < 0 )
			{
				error(0, errno, "kill: (%" PRIiPID ")", pid);
				result = 1;
			}
		}
		return result;
	}
}
