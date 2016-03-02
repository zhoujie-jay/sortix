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
 * time.c
 * Measure process running time.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

static pid_t child_pid;
static struct timespec start_time, end_time;

void statistics(void)
{
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	struct tmns tmns;
	timens(&tmns);
	struct timespec real_time = timespec_sub(end_time, start_time);
	struct timespec execute_time = timespec_sub(tmns.tmns_cutime, tmns.tmns_utime);
	struct timespec system_time = timespec_sub(tmns.tmns_cstime, tmns.tmns_stime);
	struct timespec user_time = timespec_sub(execute_time, system_time);

	fprintf(stderr, "\n");
	fprintf(stderr, "real\t%lim%li.%03li\n", (long) real_time.tv_sec / 60, (long) real_time.tv_sec % 60, real_time.tv_nsec / 1000000L);
	fprintf(stderr, "user\t%lim%li.%03li\n", (long) user_time.tv_sec / 60, (long) user_time.tv_sec % 60, user_time.tv_nsec / 1000000);
	fprintf(stderr, "sys\t%lim%li.%03li\n", (long) system_time.tv_sec / 60, (long) system_time.tv_sec % 60, system_time.tv_nsec / 1000000);
}

void signal_handler(int signum)
{
	int code;
	waitpid(child_pid, &code, 0);
	statistics();
	raise(signum);
	_exit(1);
}

int main(int argc, char* argv[])
{
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	sigset_t handleset, oldset;
	sigemptyset(&handleset);
	sigaddset(&handleset, SIGINT);
	sigaddset(&handleset, SIGQUIT);
	sigprocmask(SIG_BLOCK, &handleset, &oldset);
	if ( (child_pid = fork()) < 0 )
		error(1, errno, "fork");
	if ( !child_pid )
	{
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		if ( argc <= 1 )
			exit(0);
		execvp(argv[1], argv+1);
		error(127, errno, "%s", argv[1]);
	}
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = signal_handler;
	sa.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	int code = 0;
	sigprocmask(SIG_SETMASK, &oldset, NULL);
	waitpid(child_pid, &code, 0);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	statistics();
	if ( WIFSIGNALED(code) )
		raise(WTERMSIG(code));
	return WEXITSTATUS(code);
}
