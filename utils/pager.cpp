#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/sortix-keyboard.h>

using namespace Maxsi;

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { printf("usage: %s [FILE] ...\n", argv[0]); return 0; }
	const int HEIGHT = 25;
	const int WIDTH = 80;
	int linesleft = HEIGHT-1;
	int result = 0;
	size_t charleft = WIDTH;
	for ( int i = 1; i < argc; i++ )
	{
		int fd = ( strcmp(argv[i], "-") == 0 ) ? 0 : open(argv[i], O_RDONLY);
		if ( fd < 0 ) { result = 1; perror(argv[i]); continue; }
		const size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		while ( true )
		{
			ssize_t numbytes = read(fd, buffer, BUFFER_SIZE);
			if ( !numbytes ) { break; }
			if ( numbytes < 0 )
			{
				result = 1;
				error(0, errno, "read: %s", argv[i]);
				break;
			}
			for ( ssize_t i = 0; i < numbytes; i++ )
			{
				bool printed = false;
				char c = buffer[i];
				if ( c == '\n' ) { charleft = WIDTH; }
				bool eol = (c == '\n') || !charleft;
				if ( eol ) { charleft = WIDTH; }
				if ( eol && !linesleft )
				{
					printf("\n--pager--");
					fflush(stdout);
					uint32_t codepoint;
					bool doexit = false;
					do
					{
						unsigned method = System::Keyboard::POLL;
						codepoint = System::Keyboard::ReceiveKeystroke(method);
						if ( codepoint == Keyboard::DOWN ) { break; }
						if ( codepoint == Keyboard::PGDOWN ) { linesleft = HEIGHT-1; break; }
						if ( codepoint == 'q' ) { doexit = true; break; }
					}
					while ( codepoint != '\n' );
					printf("\r\e[J");
					if ( doexit ) { exit(result); }
					continue;
				}
				if ( eol && linesleft ) { linesleft--; }
				printf("%c", c);
				charleft--;
				if ( c == '\t' ) { charleft &= ~(4-1); }
			}
		}
		if ( fd != 0 ) { close(fd); }
	}
	return result;
}
