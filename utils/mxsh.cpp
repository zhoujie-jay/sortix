/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    mxsh.cpp
    A simple and hacky Sortix shell.

*******************************************************************************/

#include <sys/wait.h>
#include <sys/termmode.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int status = 0;

const char* getenv_safe(const char* name, const char* def = "")
{
	const char* ret = getenv(name);
	return ret ? ret : def;
}

void on_sigint(int /*signum*/)
{
	printf("^C\n");
}

void updatepwd()
{
	const size_t CWD_SIZE = 512;
	char cwd[CWD_SIZE];
	const char* wd = getcwd(cwd, CWD_SIZE);
	if ( !wd ) { wd = "?"; }
	setenv("PWD", wd, 1);
}

void updateenv()
{
	char str[128];
	struct winsize ws;
	if ( tcgetwinsize(0, &ws) == 0 )
	{
		sprintf(str, "%zu", ws.ws_col);
		setenv("COLUMNS", str, 1);
		sprintf(str, "%zu", ws.ws_row);
		setenv("LINES", str, 1);
	}
}

int runcommandline(const char** tokens, bool* exitexec, bool interactive)
{
	int result = 127;
	size_t cmdnext = 0;
	size_t cmdstart;
	size_t cmdend;
	bool lastcmd = false;
	int pipein = 0;
	int pipeout = 1;
	int pipeinnext = 0;
	char** argv;
	size_t cmdlen;
	const char* execmode;
	const char* outputfile;
	pid_t childpid;
	pid_t pgid = -1;
	bool internal;
	int internalresult;
readcmd:
	// Collect any pending zombie processes.
	while ( 0 < waitpid(-1, NULL, WNOHANG) );

	cmdstart = cmdnext;
	for ( cmdend = cmdstart; true; cmdend++ )
	{
		const char* token = tokens[cmdend];
		if ( !token ||
		     strcmp(token, ";") == 0 ||
		     strcmp(token, "&") == 0 ||
		     strcmp(token, "|") == 0 ||
		     strcmp(token, ">") == 0 ||
		     strcmp(token, ">>") == 0 ||
		     false )
		{
			break;
		}
	}

	cmdlen = cmdend - cmdstart;
	if ( !cmdlen ) { fprintf(stderr, "expected command\n"); goto out; }
	execmode = tokens[cmdend];
	if ( !execmode ) { lastcmd = true; execmode = ";"; }
	tokens[cmdend] = NULL;

	if ( strcmp(execmode, "|") == 0 )
	{
		int pipes[2];
		if ( pipe(pipes) ) { perror("pipe"); goto out; }
		if ( pipeout != 1 ) { close(pipeout); } pipeout = pipes[1];
		if ( pipeinnext != 0 ) { close(pipeinnext); } pipeinnext = pipes[0];
	}

	outputfile = NULL;
	if ( strcmp(execmode, ">") == 0 || strcmp(execmode, ">>") == 0 )
	{
		outputfile = tokens[cmdend+1];
		if ( !outputfile ) { fprintf(stderr, "expected filename\n"); goto out; }
		const char* nexttok = tokens[cmdend+2];
		if ( nexttok ) { fprintf(stderr, "too many filenames\n"); goto out; }
	}

	cmdnext = cmdend + 1;
	argv = (char**) (tokens + cmdstart);

	updateenv();
	char statusstr[32];
	sprintf(statusstr, "%i", status);
	setenv("?", statusstr, 1);

	for ( char** argp = argv; *argp; argp++ )
	{
		char* arg = *argp;
		if ( arg[0] != '$' )
			continue;
		arg = getenv(arg+1);
		if ( !arg )
			arg = (char*) "";
		*argp = arg;
	}

	internal = false;
	internalresult = 0;
	if ( strcmp(argv[0], "cd") == 0 )
	{
		internal = true;
		const char* newdir = getenv_safe("HOME", "/");
		if ( argv[1] ) { newdir = argv[1]; }
		if ( chdir(newdir) )
		{
			error(0, errno, "cd: %s", newdir);
			internalresult = 1;
		}
		updatepwd();
	}
	if ( strcmp(argv[0], "exit") == 0 )
	{
		int exitcode = argv[1] ? atoi(argv[1]) : 0;
		*exitexec = true;
		return exitcode;
	}
	if ( strcmp(argv[0], "unset") == 0 )
	{
		internal = true;
		unsetenv(argv[1] ? argv[1] : "");
	}
	if ( strcmp(argv[0], "clearenv") == 0 )
	{
		internal = true;
		clearenv();
	}

	childpid = internal ? getpid() : fork();
	if ( childpid < 0 ) { perror("fork"); goto out; }
	if ( childpid )
	{
		if ( !internal )
		{
			if ( pgid == -1 )
				pgid = childpid;
			setpgid(childpid, pgid);
		}

		if ( pipein != 0 ) { close(pipein); pipein = 0; }
		if ( pipeout != 1 ) { close(pipeout); pipeout = 1; }
		if ( pipeinnext != 0 ) { pipein = pipeinnext; pipeinnext = 0; }

		if ( strcmp(execmode, "&") == 0 && !tokens[cmdnext] )
		{
			result = 0; goto out;
		}

		if ( strcmp(execmode, "&") == 0 )
			pgid = -1;

		if ( strcmp(execmode, "&") == 0 || strcmp(execmode, "|") == 0 )
		{
			goto readcmd;
		}

		status = internalresult;
		int exitstatus;
		tcsetpgrp(0, pgid);
		if ( !internal && waitpid(childpid, &exitstatus, 0) < 0 )
		{
			perror("waitpid");
			return 127;
		}
		tcsetpgrp(0, getpgid(0));

		// TODO: HACK: Most signals can't kill processes yet.
		if ( WEXITSTATUS(exitstatus) == 128 + SIGINT )
			printf("^C\n");
		if ( WTERMSIG(status) == SIGKILL )
			printf("Killed\n");

		status = WEXITSTATUS(exitstatus);

		if ( strcmp(execmode, ";") == 0 && tokens[cmdnext] && !lastcmd )
		{
			goto readcmd;
		}

		result = status;
		goto out;
	}

	setpgid(0, pgid != -1 ? pgid : 0);

	if ( pipeinnext != 0 ) { close(pipeinnext); }

	if ( pipein != 0 )
	{
		close(0);
		dup(pipein);
		close(pipein);
	}

	if ( pipeout != 1 )
	{
		close(1);
		dup(pipeout);
		close(pipeout);
	}

	if ( outputfile )
	{
		close(1);
		int flags = O_CREAT | O_WRONLY | O_APPEND;
		if ( strcmp(execmode, ">") == 0 ) { flags |= O_TRUNC; }
		if ( open(outputfile, flags, 0666) < 0 )
		{
			error(127, errno, "%s", outputfile);
		}
	}

	if ( !strcmp(argv[0], "env") )
	{
		for ( size_t i = 0; i < envlength(); i++ )
		{
			printf("%s\n", getenvindexed(i));
		}
		exit(0);
	}

	execvp(argv[0], argv);
	if ( interactive )
	{
		int errno_saved = errno;
		execlp("command-not-found", "command-not-found", argv[0], NULL);
		errno = errno_saved;
	}
	error(127, errno, "%s", argv[0]);
	return 127;

out:
	if ( pipein != 0 ) { close(pipein); }
	if ( pipeout != 1 ) { close(pipeout); }
	if ( pipeinnext != 0 ) { close(pipeout); }
	return result;
}

int get_and_run_command(FILE* fp, const char* fpname, bool interactive,
                        bool exit_on_error, bool* exitexec)
{
	int fd = fileno(fp);

	if ( interactive )
	{
		unsigned termmode = TERMMODE_UNICODE
		                  | TERMMODE_SIGNAL
		                  | TERMMODE_UTF8
		                  | TERMMODE_LINEBUFFER
		                  | TERMMODE_ECHO;
		settermmode(fd, termmode);
		const char* print_username = getlogin();
		if ( !print_username )
			print_username = getuid() == 0 ? "root" : "?";
		const char* print_hostname = getenv_safe("HOSTNAME", "sortix");
		const char* print_dir = getenv_safe("PWD", "?");
		const char* home_dir = getenv_safe("HOME", "");
		size_t home_dir_len = strlen(home_dir);
		printf("\e[32m");
		printf("%s", print_username);
		printf("@");
		printf("%s", print_hostname);
		printf(" ");
		printf("\e[36m");
		if ( home_dir_len && strncmp(print_dir, home_dir, home_dir_len) == 0 )
			printf("~%s", print_dir + home_dir_len);
		else
			printf("%s", print_dir);
		printf(" ");
		printf("#");
		printf("\e[37m ");
		fflush(stdout);
	}

	static char* command = NULL;
	static size_t commandlen = 1024;
	if ( !command )
		commandlen = 1024,
		command = (char*) malloc((commandlen+1) * sizeof(char));
	if ( !command )
		error(64, errno, "malloc");

	size_t commandused = 0;
	bool commented = false;
	bool escaped = false;
	while (true)
	{
		char c;
		ssize_t bytesread = read(fd, &c, sizeof(c));
		if ( bytesread < 0 && errno == EINTR )
			return status;
		if ( bytesread < 0 )
			error(64, errno, "read %s", fpname);
		if ( !bytesread )
		{
			if ( !interactive )
			{
				*exitexec = true;
				return status;
			}
			const char* init_pid_str = getenv("INIT_PID");
			if ( !init_pid_str)
				init_pid_str = "1";
			pid_t init_pid = (pid_t) atol(init_pid_str);
			if ( !init_pid )
				init_pid = 1;
			if ( getppid() == init_pid )
			{
				printf("\nType exit to shutdown the system.\n");
				return status;
			}
			printf("exit\n");
			*exitexec = true;
			return status;
		}
		if ( !c ) { continue; }
		if ( c == '\n' && !escaped )
			break;
		if ( commented )
			continue;
		if ( c == '\\' && !escaped )
		{
			escaped = true;
			continue;
		}
		if ( !escaped )
		{
			if ( c == '#' )
			{
				commented = true;
				continue;
			}
		}
		escaped = false;
		if ( commandused == commandlen )
		{
			size_t newlen = commandlen * 2;
			size_t newsize = (newlen+1) * sizeof(char);
			char* newcommand = (char*) realloc(command, newsize);
			if ( !newcommand )
				error(64, errno, "realloc");
			command = newcommand;
			commandlen = newsize;
		}
		command[commandused++] = c;
	}

	command[commandused] = '\0';

	if ( command[0] == '\0' )
		return status;

	if ( strchr(command, '=') && !strchr(command, ' ') && !strchr(command, '\t') )
	{
		if ( putenv(strdup(command)) )
			error(1, errno, "putenv");
		return status = 0;
	}

	int argc = 0;
	const size_t ARGV_MAX_LENGTH = 2048;
	const char* argv[ARGV_MAX_LENGTH];
	argv[0] = NULL;

	bool lastwasspace = true;
	escaped = false;
	for ( size_t i = 0; i <= commandused; i++ )
	{
		switch ( command[i] )
		{
			case '\\':
				if ( !escaped )
				{
					memmove(command + i, command + i + 1, commandused+1 - (i-1));
					i--;
					commandused--;
					escaped = true;
					break;
				}
			case '\0':
			case ' ':
			case '\t':
			case '\n':
				if ( !command[i] || !escaped )
				{
					command[i] = 0;
					lastwasspace = true;
					break;
				}
			default:
				escaped = false;
				if ( lastwasspace )
				{
					if ( argc == ARGV_MAX_LENGTH  )
					{
						fprintf(stderr, "argv max length of %zu entries hit!\n",
						        ARGV_MAX_LENGTH);
						abort();
					}
					argv[argc++] = command + i;
				}
				lastwasspace = false;
		}
	}

	if ( !argv[0] )
		return status;

	argv[argc] = NULL;
	status = runcommandline(argv, exitexec, interactive);
	if ( status && exit_on_error )
		*exitexec = true;
	return status;
}

void load_argv_variables(int argc, char* argv[])
{
	char varname[sizeof(int) * 3];
	for ( int i = 0; i < argc; i++ )
		sprintf(varname, "%i", i),
		setenv(varname, argv[i], 1);
}

int run(FILE* fp, int argc, char* argv[], const char* name, bool interactive,
        bool exit_on_error)
{
	load_argv_variables(argc, argv);
	bool exitexec = false;
	int exitstatus;
	do
		exitstatus = get_and_run_command(fp, name, interactive, exit_on_error,
		                                 &exitexec);
	while ( !exitexec );
	return exitstatus;
}

int run_interactive(int argc, char* argv[], bool exit_on_error)
{
	signal(SIGINT, on_sigint);
	return run(stdin, argc, argv, "<stdin>", true, exit_on_error);
}

int run_stdin(int argc, char* argv[], bool exit_on_error)
{
	return run(stdin, argc, argv, "<stdin>", false, exit_on_error);
}

int run_string(int argc, char* argv[], const char* str, bool exit_on_error)
{
	// TODO: Implement fmemopen and open_memstream.
	char unique_ish[64];
	snprintf(unique_ish, sizeof(unique_ish), "/tmp/shinput.%i", getpid());
	FILE* fp = fopen(unique_ish, "w");
	if ( !fp ) { error(0, errno, "write-open: %s", unique_ish); return 1; }
	if ( fputs(str, fp) == EOF ) { fclose(fp); error(0, errno, "write: %s", unique_ish); return 1; }
	if ( fputs("\n", fp) == EOF ) { fclose(fp); error(0, errno, "write: %s", unique_ish); return 1; }
	fclose(fp);
	fp = fopen(unique_ish, "r");
	if ( !fp ) { error(0, errno, "read-open: %s", unique_ish); return 1; }
	int ret = run(fp, argc, argv, "<command-line>", false, exit_on_error);
	fclose(fp);
	return ret;
}

int run_script(const char* path, int argc, char* argv[], bool exit_on_error)
{
	FILE* fp = fopen(path, "r");
	if ( !fp ) { error(0, errno, "%s", path); return 127; }
	int ret = run(fp, argc, argv, path, false, exit_on_error);
	fclose(fp);
	return ret;
}

int main(int argc, char* argv[])
{
	bool exit_on_error = false;
	if ( 2 <= argc && !strcmp(argv[1], "-e") )
	{
		exit_on_error = true;
		argc--;
		for ( int i = 1; i < argc; i++ )
			argv[i] = argv[i+1];
		argv[argc] = NULL;
	}
	char pidstr[32];
	char ppidstr[32];
	sprintf(pidstr, "%i", getpid());
	sprintf(ppidstr, "%i", getppid());
	setenv("SHELL", argv[0], 1);
	setenv("$", pidstr, 1);
	setenv("PPID", ppidstr, 1);
	setenv("?", "0", 1);
	updatepwd();
	if ( 2 <= argc && !strcmp(argv[1], "-c") )
		return run_string(argc, argv, argv[2], exit_on_error);
	if ( 1 < argc )
		return run_script(argv[1], argc-1, argv+1, exit_on_error);
	if ( isatty(0) )
		return run_interactive(argc, argv, exit_on_error);
	return run_stdin(argc, argv, exit_on_error);
}
