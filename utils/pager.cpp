#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>

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
					settermmode(0, TERMMODE_KBKEY | TERMMODE_SIGNAL);
					bool doexit = false;
					int kbkey;
					do
					{
						uint32_t codepoint;
						ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
						if ( !numbytes ) { exit(0); }
						if ( numbytes < 0 ) { error(1, errno, "read(stdin)"); }
						if ( numbytes < sizeof(codepoint) ) { error(1, errno, "bad stdin"); }
						if ( !(kbkey = KBKEY_DECODE(codepoint)) ) { continue; }
						if ( kbkey == KBKEY_DOWN ) { break; }
						if ( kbkey == KBKEY_PGDOWN ) { linesleft = HEIGHT-1; break; }
						if ( kbkey == -KBKEY_Q ) { doexit = true; break; }						
					} while ( kbkey != KBKEY_ENTER );
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
