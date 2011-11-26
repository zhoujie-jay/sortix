#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>
#include <libmaxsi/sortix-keyboard.h>
#include <libmaxsi/string.h>

using namespace Maxsi;

int status = 0;

void command()
{
	const size_t CWD_SIZE = 512;
	char cwd[CWD_SIZE];
	const char* wd = getcwd(cwd, CWD_SIZE);
	if ( !wd ) { wd = "?"; }

	printf("root@sortix %s # ", wd);	

	const size_t commandsize = 128;
	char command[commandsize + 1];
	size_t commandused = 0;

	while (true)
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Maxsi::Keyboard::DEPRESSED ) { continue; }
		if ( codepoint >= 0x80 ) { continue; }

		if ( codepoint == '\b' )
		{
			if ( 0 < commandused ) { printf("\b"); commandused--; }
			continue;
		}

		if ( commandsize <= commandused && codepoint != '\n' ) { continue; }

		char msg[2]; msg[0] = codepoint; msg[1] = '\0';
		printf("%s", msg);

		if ( codepoint == '\n' ) { command[commandused] = '\0'; break; }

		command[commandused++] = codepoint;
	}

	if ( command[0] == '\0' ) { return; }

	if ( String::Compare(command, "$?") == 0 ) { printf("%u\n", status); status = 0; return; }
	if ( String::Compare(command, "$$") == 0 ) { printf("%u\n", Process::GetPID()); status = 0; return; }
	if ( String::Compare(command, "$PPID") == 0 ) { printf("%u\n", Process::GetParentPID()); status = 0; return; }

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

	if ( strcmp(argv[0], "exit") == 0 )
	{
		const char* status = "1";
		if ( 1 < argc ) { status = argv[1]; }
		exit(atoi(status));
	}

	if ( strcmp(argv[0], "cd") == 0 )
	{
		status = 0;
		const char* newdir = "/";
		if ( 1 < argc ) { newdir = argv[1]; }
		if ( chdir(newdir) )
		{
			error(0, errno, "cd: %s", newdir);
			status = 1;
		}
		return;
	}

	pid_t child = fork();
	if ( child < 0 ) { perror("fork"); status = 1; return; }
	if ( child != 0 )
	{
		pid_t childpid = wait(&status);
		return;
	}

	if ( 3 <= argc )
	{
		if ( strcmp(argv[argc-2], ">") == 0 )
		{
			const char* file = argv[argc-1];
			int outfd = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_APPEND);
			if ( outfd < 0 ) { error(127, errno, "%s", file); exit(127); }
			close(1);
			dup(outfd);
			close(outfd);
			argc -= 2;
		}
	}

	// Replace the current process with another process image.
	Process::Execute(argv[0], argc, argv);

	// This is clever. This only happens if the program didn't change.
	error(127, errno, "%s", argv[0]);
}

int main(int argc, char* argv[])
{
	while ( true ) { command(); }
}

