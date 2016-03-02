/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * login.c
 * Authenticates users.
 */

#include <sys/termmode.h>
#include <sys/wait.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ioleast.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// TODO: The Sortix <limits.h> doesn't expose this at the moment.
#if !defined(HOST_NAME_MAX) && defined(__sortix__)
#include <sortix/limits.h>
#endif

#include "login.h"

static void on_interrupt_signal(int signum)
{
	if ( signum == SIGINT )
		dprintf(1, "^C");
	if ( signum == SIGQUIT )
		dprintf(1, "^\\");
}

bool check_real(const char* username, const char* password)
{
	char fakehashbuf[128];
	char goodhashbuf[128];
	size_t fakematch = 0;
	size_t goodmatch = 0;
	const char* fakehash = NULL;
	const char* goodhash = NULL;
	setpwent();
	struct passwd* pwd;
	while ( (errno = 0, pwd = getpwent()) )
	{
		if ( !strcmp(username, pwd->pw_name) )
		{
			strlcpy(goodhashbuf, pwd->pw_passwd, sizeof(goodhashbuf));
			goodhash = goodhashbuf;
			goodmatch++;
		}
		else
		{
			strlcpy(fakehashbuf, pwd->pw_passwd, sizeof(fakehashbuf));
			fakehash = fakehashbuf;
			fakematch++;
		}
	}
	int errnum = errno;
	endpwent();
	if ( errnum != 0 )
		return errno = errnum, false;
	if ( 1 < goodmatch )
		return errno = EACCES, false;
	errno = 0;
	(void) fakehash;
	return crypt_checkpass(password, goodhash) == 0;
}

bool check_begin(struct check* chk,
                 const char* username,
                 const char* password,
                 bool restrict_termmode)
{
	memset(chk, 0, sizeof(*chk));
	if ( tcgetattr(0, &chk->tio) )
		return false;
	int pipe_fds[2];
	if ( pipe2(pipe_fds, O_CLOEXEC) < 0 )
		return false;
	sigset_t sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	sigprocmask(SIG_BLOCK, &sigttou, &chk->oldset);
	if ( (chk->pid = fork()) < 0 )
		return close(pipe_fds[0]), close(pipe_fds[1]), false;
	int success = -2;
	if ( chk->pid == 0 )
	{
		sigdelset(&chk->oldset, SIGINT);
		sigdelset(&chk->oldset, SIGQUIT);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		unsigned int termmode = TERMMODE_UNICODE | TERMMODE_SIGNAL |
		                        TERMMODE_UTF8 | TERMMODE_LINEBUFFER |
		                        TERMMODE_ECHO;
		if ( restrict_termmode )
			termmode = TERMMODE_SIGNAL;
		if ( setpgid(0, 0) < 0 ||
		     close(pipe_fds[0]) < 0 ||
		     tcsetpgrp(0, getpgid(0)) ||
		     sigprocmask(SIG_SETMASK, &chk->oldset, NULL) < 0 ||
		     settermmode(0, termmode) < 0 ||
		     !check_real(username, password) ||
		     write(pipe_fds[1], &success, sizeof(success)) < 0 )
		{
			assert(1 <= errno);
			write(pipe_fds[1], &errno, sizeof(errno));
		}
		_exit(0);
	}
	close(pipe_fds[1]);
	chk->pipe = pipe_fds[0];
	return true;
}

bool check_end(struct check* chk, bool* result, bool try)
{
	if ( try && !chk->pipe_nonblock )
	{
		fcntl(chk->pipe, F_SETFL, fcntl(chk->pipe, F_GETFL) | O_NONBLOCK);
		chk->pipe_nonblock = true;
	}
	while ( chk->errnum_done < sizeof(chk->errnum_bytes) )
	{
		ssize_t amount = read(chk->pipe, chk->errnum_bytes + chk->errnum_done,
		                      sizeof(chk->errnum_bytes) - chk->errnum_done);
		if ( amount <= 0 )
		{
			if ( amount == 0 )
				errno = EOF;
			break;
		}
		chk->errnum_done += amount;
	}
	int code;
	pid_t wait_ret = waitpid(chk->pid, &code, try ? WNOHANG : 0);
	if ( try && wait_ret == 0 )
		return false;
	tcsetattr(0, TCSAFLUSH, &chk->tio);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &chk->oldset, NULL);
	if ( wait_ret < 0 )
		return *result = false, true;
	if ( chk->errnum_done < sizeof(chk->errnum_bytes) )
		chk->errnum = EEOF;
	if ( WIFSIGNALED(code) )
		chk->errnum = EINTR;
	else if ( !(WIFEXITED(code) && WEXITSTATUS(code) == 0) )
		chk->errnum = EINVAL;
	int success = -2;
	if ( chk->errnum < 1 && chk->errnum != success )
		chk->errnum = EINVAL;
	if ( chk->errnum != success )
		return errno = chk->errnum, *result = false, true;
	return *result = true, true;
}

bool check(const char* username, const char* password)
{
	struct check chk;
	if ( !check_begin(&chk, username, password, false) )
		return false;
	bool result;
	check_end(&chk, &result, false);
	return result;
}

static int setcloexecfrom(int from)
{
	int fd = from - 1;
	while ( (fd = fcntl(fd, F_NEXTFD)) != -1 )
	{
		int flags = fcntl(fd, F_GETFD);
		if ( flags < 0 )
			return -1;
		if ( !(flags & FD_CLOEXEC) && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0 )
			return -1;
	}
	return 0;
}

bool login(const char* username)
{
	char login_pid[sizeof(pid_t) * 3];
	snprintf(login_pid, sizeof(login_pid), "%" PRIiPID, getpid());
	struct passwd* pwd = getpwnam(username);
	if ( !pwd )
		return false;
	struct termios tio;
	if ( tcgetattr(0, &tio) )
		return false;
	int pipe_fds[2];
	if ( pipe2(pipe_fds, O_CLOEXEC) < 0 )
		return false;
	sigset_t oldset, sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	sigprocmask(SIG_BLOCK, &sigttou, &oldset);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return close(pipe_fds[0]), close(pipe_fds[1]), false;
	if ( child_pid == 0 )
	{
		sigdelset(&oldset, SIGINT);
		sigdelset(&oldset, SIGQUIT);
		sigdelset(&oldset, SIGTSTP);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		(void) (
		setpgid(0, 0) < 0 ||
		close(pipe_fds[0]) < 0 ||
		setgid(pwd->pw_gid) < 0 ||
		setuid(pwd->pw_uid) < 0 ||
		setenv("LOGIN_PID", login_pid, 1) < 0 ||
		setenv("LOGNAME", pwd->pw_name, 1) < 0 ||
		setenv("USER", pwd->pw_name, 1) < 0 ||
		chdir(pwd->pw_dir) < 0 ||
		setenv("HOME", pwd->pw_dir, 1) < 0 ||
		setenv("SHELL", pwd->pw_shell, 1) < 0 ||
		close(0) < 0 ||
		close(1) < 0 ||
		close(2) < 0 ||
		open("/dev/tty", O_RDONLY) != 0 ||
		open("/dev/tty", O_WRONLY) != 1 ||
		open("/dev/tty", O_WRONLY) != 2 ||
		setcloexecfrom(3) < 0 ||
		tcsetpgrp(0, getpgid(0)) ||
		sigprocmask(SIG_SETMASK, &oldset, NULL) < 0 ||
		settermmode(0, TERMMODE_NORMAL) < 0 ||
		execlp(pwd->pw_shell, pwd->pw_shell, (const char*) NULL));
		write(pipe_fds[1], &errno, sizeof(errno));
		_exit(127);
	}
	close(pipe_fds[1]);
	int errnum;
	if ( readall(pipe_fds[0], &errnum, sizeof(errnum)) < (ssize_t) sizeof(errnum) )
		errnum = 0;
	close(pipe_fds[0]);
	int child_status;
	if ( waitpid(child_pid, &child_status, 0) < 0 )
		errnum = errno;
	tcsetattr(0, TCSAFLUSH, &tio);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &oldset, NULL);
	dprintf(1, "\e[H\e[2J");
	fsync(1);
	if ( errnum != 0 )
		return errno = errnum, false;
	return true;
}

static bool read_terminal_line(char* buffer, size_t size)
{
	assert(size);
	size--;
	sigset_t intset;
	sigemptyset(&intset);
	sigaddset(&intset, SIGINT);
	sigaddset(&intset, SIGQUIT);
	bool newline = false;
	size_t sofar = 0;
	while ( !newline && sofar < size )
	{
		sigset_t oldset;
		sigprocmask(SIG_UNBLOCK, &intset, &oldset);
		ssize_t amount = read(0, buffer + sofar, size - sofar);
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		if ( amount <= 0 )
			return false;
		for ( ssize_t i = 0; i < amount; i++ )
		{
			if ( buffer[sofar + i] != '\n' )
				continue;
			newline = true;
			amount = i;
			break;
		}
		sofar += amount;
	}
	while ( !newline )
	{
		char c;
		if ( read(0, &c, 1) <= 0 )
			return false;
		newline = c == '\n';
	}
	buffer[sofar] = '\0';
	return true;
}

int textual(void)
{
	unsigned int termmode = TERMMODE_UNICODE | TERMMODE_SIGNAL | TERMMODE_UTF8 |
	                        TERMMODE_LINEBUFFER | TERMMODE_ECHO;
	if ( settermmode(0, termmode) < 0 )
		err(2, "settermmode");
	unsigned int pw_termmode = termmode & ~(TERMMODE_ECHO);

	while ( true )
	{
		char hostname[HOST_NAME_MAX + 1];
		hostname[0] = '\0';
		gethostname(hostname, sizeof(hostname));
		printf("%s login: ", hostname);
		fflush(stdout);
		char username[256];
		errno = 0;
		if ( !read_terminal_line(username, sizeof(username)) )
		{
			printf("\n");
			if ( errno && errno != EINTR )
			{
				warn("fgets");
				sleep(1);
			}
			printf("\n");
			continue;
		}

		if ( !strcmp(username, "exit") )
			exit(0);
		if ( !strcmp(username, "poweroff") )
			exit(0);
		if ( !strcmp(username, "reboot") )
			exit(1);

		if ( settermmode(0, pw_termmode) < 0 )
			err(2, "settermmode");
		printf("Password (will not echo): ");
		fflush(stdout);
		char password[256];
		errno = 0;
		bool password_success = read_terminal_line(password, sizeof(password));
		printf("\n");
		if ( settermmode(0, termmode) < 0 )
			err(2, "settermmode");
		if ( !password_success )
		{
			if ( errno && errno != EINTR )
			{
				warn("fgets");
				sleep(1);
			}
			printf("\n");
			continue;
		}

		bool result = check(username, password);
		explicit_bzero(password, sizeof(password));
		if ( !result )
		{
			const char* msg = "Invalid username/password";
			if ( errno != EACCES )
				msg = strerror(errno);
			printf("%s\n", msg);
			printf("\n");
			continue;
		}

		if ( !login(username) )
		{
			warn("logging in as %s", username);
			printf("\n");
			continue;
		}
	}

	return 0;
}

int main(void)
{
	setlocale(LC_ALL, "");
	if ( getuid() != 0 )
		errx(2, "must be user root");
	if ( getgid() != 0 )
		errx(2, "must be group root");
	if ( !isatty(0) )
	{
		close(0);
		if ( open("/dev/tty", O_RDONLY) != 0 )
			err(2, "/dev/tty");
	}
	if ( !isatty(1) )
	{
		close(1);
		if ( open("/dev/tty", O_WRONLY) != 0 )
			err(2, "/dev/tty");
	}
	if ( !isatty(2) )
	{
		if ( dup2(1, 2) < 0 )
			err(2, "dup2");
	}
	if ( tcgetpgrp(0) != getpgid(0) )
		errx(2, "must be in foreground process group");
	if ( getpgid(0) != getpid() )
		errx(2, "must be progress group leader");
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_interrupt_signal;
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGQUIT);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaddset(&sa.sa_mask, SIGTSTP);
	sigprocmask(SIG_BLOCK, &sa.sa_mask, NULL);
	int result = -1;
	if ( result == -1 )
		result = graphical();
	if ( result == -1 )
		result = textual();
	return result;
}
