#include <stdio.h>
#include <libmaxsi/process.h>
#include <libmaxsi/sortix-keyboard.h>

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
		uint32_t codepoint = System::Keyboard::ReceieveKeystroke(method);

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

	// Replace the current process with another process image.
	Process::Execute(command, 0, NULL);

	// This is clever. This only happens if the program didn't change.
	printf("%s: command not found\n", command);
}

int main(int argc, char* argv[])
{
	while ( true ) { command(); }
}

