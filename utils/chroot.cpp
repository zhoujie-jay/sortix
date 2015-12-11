/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    chroot.cpp
    Runs a process with another root directory.

*******************************************************************************/

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fsmarshall.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	fprintf(fp, "Usage: %s [OPTION]... ROOT [CMD] [ARGUMENT...]\n", argv0);
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

static char* mount_point_dev;

static void unmount_handler(int signum)
{
	(void) signum;
	if ( mount_point_dev )
	{
		unmount(mount_point_dev, 0);
		mount_point_dev = NULL;
	}
	raise(signum);
}

int main(int argc, char* argv[])
{
	bool devices = false;
	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			break; // Intentionally not continue.
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'd': devices = true; break;
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( argc < 2 )
		error(1, 0, "missing operand, expected new root directory");

	if ( devices )
	{
		if ( asprintf(&mount_point_dev, "%s/dev", argv[1]) < 0 )
			error(1, errno, "asprintf: `%s/dev'", argv[1]);

		// Create a device directory in the root filesystem.
		mkdir(mount_point_dev, 0755);

		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = unmount_handler;
		sa.sa_flags = SA_RESETHAND;
		sigaction(SIGHUP, &sa, NULL);
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGQUIT, &sa, NULL);

		// Mount the current device directory inside the new root filesystem.
		int old_dev_fd = open("/dev", O_DIRECTORY | O_RDONLY);
		int new_dev_fd = open(mount_point_dev, O_DIRECTORY | O_RDONLY);
		fsm_fsbind(old_dev_fd, new_dev_fd, 0);
		close(new_dev_fd);
		close(old_dev_fd);

		sigset_t oldset, sigs;
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGHUP);
		sigaddset(&sigs, SIGINT);
		sigaddset(&sigs, SIGQUIT);
		sigprocmask(SIG_BLOCK, &sigs, &oldset);
		pid_t child_pid = fork();
		if ( child_pid < 0 )
		{
			int errnum = errno;
			unmount(mount_point_dev, 0);
			mount_point_dev = NULL;
			sigprocmask(SIG_SETMASK, &oldset, NULL);
			error(1, errnum, "fork");
		}
		if ( child_pid != 0 )
		{
			sigprocmask(SIG_SETMASK, &oldset, NULL);
			int code;
			waitpid(child_pid, &code, 0);
			sigprocmask(SIG_BLOCK, &sigs, &oldset);
			unmount(mount_point_dev, 0);
			sigprocmask(SIG_SETMASK, &oldset, NULL);
			mount_point_dev = NULL;
			if ( WIFEXITED(code) )
				return WEXITSTATUS(code);
			raise(WTERMSIG(code));
			return 128 + WTERMSIG(code);
		}
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		sigprocmask(SIG_SETMASK, &oldset, NULL);
	}

	if ( chroot(argv[1]) != 0 )
		error(1, errno, "`%s'", argv[1]);

	if ( chdir("/.") != 0 )
		error(1, errno, "chdir: `%s/.'", argv[1]);

	char* default_argv[] = { (char*) "sh", (char*) NULL };

	char** exec_argv = 3 <= argc ? argv + 2 : default_argv;
	execvp(exec_argv[0], exec_argv);

	error(0, errno, "`%s'", exec_argv[0]);
	_exit(127);
}
