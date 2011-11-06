#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>
#include <libmaxsi/sortix-keyboard.h>
#include <libmaxsi/string.h>

using namespace Maxsi;

void command()
{
	printf("root@sortix / # ");	

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
		printf(msg);

		if ( codepoint == '\n' ) { command[commandused] = '\0'; break; }

		command[commandused++] = codepoint;
	}

	if ( command[0] == '\0' ) { return; }

	if ( String::Compare(command, "$$") == 0 ) { printf("%u\n", Process::GetPID()); return; }
	if ( String::Compare(command, "$PPID") == 0 ) { printf("%u\n", Process::GetParentPID()); return; }
	if ( String::Compare(command, "exit") == 0 ) { exit(0); return; }

	pid_t child = fork();
	if ( child < 0 ) { printf("fork failed\n"); return; }
	if ( child != 0 )
	{
		int status;
		pid_t childpid = wait(&status);
		return;
	}

	// Replace the current process with another process image.
	Process::Execute(command, 0, NULL);

	// This is clever. This only happens if the program didn't change.
	printf("%s: command not found\n", command);

	exit(127);
}

int main(int argc, char* argv[])
{
	while ( true ) { command(); }
}

