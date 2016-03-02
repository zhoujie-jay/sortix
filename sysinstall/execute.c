/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
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
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <err.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "execute.h"

int execute(const char* const* argv, const char* flags, ...)
{
	bool _exit_instead = false;
	bool exit_on_failure = false;
	bool foreground = false;
	bool gid_set = false;
	bool raw_exit_code = false;
	bool uid_set = false;
	bool quiet = false;
	bool quiet_stderr = false;
	gid_t gid = 0;
	uid_t uid = 0;
	va_list ap;
	va_start(ap, flags);
	for ( size_t i = 0; flags[i]; i++ )
	{
		switch ( flags[i] )
		{
		case '_': _exit_instead = true; break;
		case 'e': exit_on_failure = true; break;
		case 'f': foreground = true; break;
		case 'g': gid_set = true; gid = va_arg(ap, gid_t); break;
		case 'r': raw_exit_code = true; break;
		case 'u': uid_set = true; uid = va_arg(ap, uid_t); break;
		case 'q': quiet = true; break;
		case 'Q': quiet_stderr = true; break;
		}
	}
	va_end(ap);
	sigset_t oldset, sigttou;
	if ( foreground )
	{
		sigemptyset(&sigttou);
		sigaddset(&sigttou, SIGTTOU);
	}
	pid_t child_pid = fork();
	if ( child_pid < 0 )
	{
		warn("fork");
		if ( exit_on_failure )
			(_exit_instead ? _exit : exit)(2);
		return -1;
	}
	if ( child_pid == 0 )
	{
		if ( gid_set )
		{
			setegid(gid);
			setgid(gid);
		}
		if ( uid_set )
		{
			seteuid(uid);
			setuid(uid);
		}
		if ( foreground )
		{
			setpgid(0, 0);
			sigprocmask(SIG_BLOCK, &sigttou, &oldset);
			tcsetpgrp(0, getpgid(0));
			sigprocmask(SIG_SETMASK, &oldset, NULL);
		}
		if ( quiet )
		{
			close(1);
			open("/dev/null", O_WRONLY);
		}
		if ( quiet_stderr )
		{
			close(2);
			open("/dev/null", O_WRONLY);
		}
		execvp(argv[0], (char* const*) argv);
		warn("%s", argv[0]);
		_exit(127);
	}
	int code;
	waitpid(child_pid, &code, 0);
	if ( foreground )
	{
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
	}
	if ( exit_on_failure )
	{
		if ( !WIFEXITED(code) || WEXITSTATUS(code) != 0 )
			(_exit_instead ? _exit : exit)(2);
	}
	int exit_status;
	if ( WIFEXITED(code) )
		exit_status = WEXITSTATUS(code);
	else
		exit_status = 128 + WTERMSIG(code);
	return raw_exit_code ? code : exit_status;
}
