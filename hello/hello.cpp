#include <stdio.h>
#include <libmaxsi/sortix-keyboard.h>

int main(int argc, char* argv[])
{
	printf("\e[m\e[0J");
	printf("Hello World: Welcome to Sortix! This is a program running in user-space.\n"
	       "This program is a probably the worst text editor ever made.\n"
	       "You are currently using the buggy USA keyboard layout.\n"
	       "This terminal is controlled using ANSI Escape Sequences:\n"
	       " - Type \e[32mESC [ 2 J\e[m to clear the screen\n"
	       );

	while (true)
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceieveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Maxsi::Keyboard::DEPRESSED ) { continue; }
		if ( codepoint == Maxsi::Keyboard::UP ) { printf("\e[A"); continue; }
		if ( codepoint == Maxsi::Keyboard::DOWN ) { printf("\e[B"); continue; }
		if ( codepoint == Maxsi::Keyboard::RIGHT ) { printf("\e[C"); continue; }
		if ( codepoint == Maxsi::Keyboard::LEFT ) { printf("\e[D"); continue; }
		if ( codepoint == Maxsi::Keyboard::ESC ) { printf("\e"); continue; }
		if ( codepoint >= 0x80 ) { continue; }

		char msg[2]; msg[0] = codepoint; msg[1] = '\0';
		printf(msg);
	}

	return 0;
}

