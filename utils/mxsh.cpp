/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <termios.h>

int status = 0;

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

int runcommandline(const char** tokens)
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
	bool internal;
	int internalresult;
readcmd:
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

	internal = false;
	internalresult = 0;
	if ( strcmp(argv[0], "cd") == 0 )
	{
		internal = true;
		const char* newdir = "/";
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
		exit(exitcode);
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
		if ( pipein != 0 ) { close(pipein); pipein = 0; }
		if ( pipeout != 1 ) { close(pipeout); pipeout = 1; }
		if ( pipeinnext != 0 ) { pipein = pipeinnext; pipeinnext = 0; }

		if ( strcmp(execmode, "&") == 0 && !tokens[cmdnext] )
		{
			result = 0; goto out;
		}

		if ( strcmp(execmode, "&") == 0 || strcmp(execmode, "|") == 0 )
		{
			goto readcmd;
		}

		status = internalresult;
		if ( !internal && waitpid(childpid, &status, 0) < 0 )
		{
			perror("waitpid");
			return 127;
		}

		if ( strcmp(execmode, ";") == 0 && tokens[cmdnext] && !lastcmd )
		{
			goto readcmd;
		}

		result = status;
		goto out;
	}

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

	updateenv();
	char statusstr[32];
	sprintf(statusstr, "%i", status);
	setenv("?", statusstr, 1);

	for ( char** argp = argv; *argp; argp++ )
	{
		char* arg = *argp;
		if ( arg[0] != '$' ) { continue; }
		arg = getenv(arg+1);
		if ( !arg ) { arg = ""; }
		*argp = arg;
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
	error(127, errno, "%s", argv[0]);
	return 127;

out:
	if ( pipein != 0 ) { close(pipein); }
	if ( pipeout != 1 ) { close(pipeout); }
	if ( pipeinnext != 0 ) { close(pipeout); }
	return result;
}

void command()
{
	unsigned termmode = TERMMODE_UNICODE
	                  | TERMMODE_SIGNAL
	                  | TERMMODE_UTF8
	                  | TERMMODE_LINEBUFFER
	                  | TERMMODE_ECHO;
	settermmode(0, termmode);

	printf("root@sortix %s # ", getenv("PWD"));
	fflush(stdout);

	const size_t commandsize = 128;
	char command[commandsize + 1];
	size_t commandused = 0;

	while (true)
	{
		char c;
		ssize_t bytesread = read(1, &c, sizeof(c));
		if ( bytesread < 0 ) { error(64, errno, "read stdin"); }
		if ( !bytesread ) { break; }
		if ( !c ) { continue; }
		if ( c == '\n' ) { break; }
		if ( commandsize <= commandused ) { continue; }
		command[commandused++] = c;
	}

	command[commandused] = '\0';

	if ( command[0] == '\0' ) { return; }

	if ( strchr(command, '=') )
	{
		if ( putenv(strdup(command)) ) { perror("putenv"); status = 1; return; }
		status = 0;
		return;
	}

	int argc = 0;
	const char* argv[256];
	argv[0] = NULL;

	bool lastwasspace = true;
	for ( size_t i = 0; i <= commandused; i++ )
	{
		switch ( command[i] )
		{
			case '\0':
			case ' ':
			case '\t':
			case '\n':
				command[i] = 0;
				lastwasspace = true;
				break;
			default:
				if ( lastwasspace ) { argv[argc++] = command + i; }
				lastwasspace = false;
		}
	}

	if ( !argv[0] ) { return; }

	argv[argc] = NULL;
	status = runcommandline(argv);
	return;
}

int main(int argc, char* argv[])
{
	char pidstr[32];
	char ppidstr[32];
	sprintf(pidstr, "%i", getpid());
	sprintf(ppidstr, "%i", getppid());
	setenv("SHELL", argv[0], 1);
	setenv("$", pidstr, 1);
	setenv("PPID", ppidstr, 1);
	setenv("?", "0", 1);
	updatepwd();
	while ( true ) { command(); }
}

