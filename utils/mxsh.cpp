#include <sys/wait.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>

int status = 0;

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
	char* const* argv;
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
	argv = (char* const*) (tokens + cmdstart);

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
	}
	if ( strcmp(argv[0], "exit") == 0 )
	{
		int exitcode = argv[1] ? atoi(argv[1]) : 0;
		exit(exitcode);
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

		int status = internalresult;
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


	execv(argv[0], argv);
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

	const size_t CWD_SIZE = 512;
	char cwd[CWD_SIZE];
	const char* wd = getcwd(cwd, CWD_SIZE);
	if ( !wd ) { wd = "?"; }

	printf("root@sortix %s # ", wd);
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

	if ( strcmp(command, "$?") == 0 ) { printf("%u\n", status); status = 0; return; }
	if ( strcmp(command, "$$") == 0 ) { printf("%u\n", getpid()); status = 0; return; }
	if ( strcmp(command, "$PPID") == 0 ) { printf("%u\n", getppid()); status = 0; return; }

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
	while ( true ) { command(); }
}

