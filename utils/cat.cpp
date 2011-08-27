#include <stdio.h>
#include <libmaxsi/sortix-keyboard.h>

int main(int argc, char* argv[])
{
	bool lastwasesc = false;

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
		if ( codepoint == Maxsi::Keyboard::ESC ) { printf("\e["); lastwasesc = true; continue; }
		if ( lastwasesc && codepoint == '[' ) { continue; }
		if ( codepoint >= 0x80 ) { continue; }

		char msg[2]; msg[0] = codepoint; msg[1] = '\0';
		printf(msg);
		lastwasesc = false;
	}

	return 0;
}

