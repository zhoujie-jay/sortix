#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <libmaxsi/sortix-keyboard.h>

int cat(int argc, char* argv[])
{
	int result = 0;

	int outfd = open("/dev/tty", O_WRONLY | O_APPEND);
	if ( outfd < 0 ) { error(0, errno, "%s", "/dev/tty"); return 1; }

	for ( int i = 1; i < argc; i++ )
	{
		int fd = open(argv[i], O_RDONLY);
		if ( fd < 0 )
		{
			error(0, errno, "%s", argv[i]);
			result = 1;
			continue;
		}

		do
		{
			const size_t BUFFER_SIZE = 255;
			char buffer[BUFFER_SIZE+1];
			ssize_t bytesread = read(fd, buffer, BUFFER_SIZE);
			if ( bytesread == 0 ) { break; }
			if ( bytesread < 0 )
			{
				error(0, errno, "read: %s", argv[i]);
				result = 1;
				break;
			}
			if ( writeall(outfd, buffer, bytesread) )
			{
				error(0, errno, "write: %s", argv[i]);
				result = 1;
				break;
			}
		} while ( true );

		close(fd);
	}

	return result;
}

int main(int argc, char* argv[])
{
	if ( 1 < argc )
	{
		return cat(argc, argv);
	}

	bool lastwasesc = false;

	while (true)
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

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

