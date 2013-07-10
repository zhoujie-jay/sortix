/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    pager.cpp
    Displays files one page at a time.

*******************************************************************************/

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <termios.h>

char* stdinargv[2];

int main(int argc, char* argv[])
{
	if ( isatty(0) && argc < 2 )
	{
		printf("usage: %s [FILE] ...\n", argv[0]);
		return 0;
	}

	if ( !isatty(0) && argc < 2 )
	{
		stdinargv[0] = argv[0];
		stdinargv[1] = (char*) "-";
		argv = stdinargv;
		argc = 2;
	}

	int stdinfd = dup(0);
	close(0);
	int ttyfd = open("/dev/tty", O_RDONLY);
	if ( ttyfd != 0 ) { perror("/dev/tty"); return 1; }
	struct winsize ws;
	if ( tcgetwinsize(0, &ws) )
		error(1, errno, "tcgetwinsize");
	const int HEIGHT = ws.ws_row;
	const int WIDTH = ws.ws_col;
	int linesleft = HEIGHT-1;
	int result = 0;
	size_t charleft = WIDTH;
	for ( int i = 1; i < argc; i++ )
	{
		int fd = ( strcmp(argv[i], "-") == 0 ) ? stdinfd : open(argv[i], O_RDONLY);
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
				char c = buffer[i];
				if ( c == '\n' ) { charleft = WIDTH; }
				bool eol = (c == '\n') || !charleft;
				if ( eol ) { charleft = WIDTH; }
				if ( eol && linesleft <= 1 )
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
						if ( numbytes < (ssize_t) sizeof(codepoint) ) { error(1, errno, "bad stdin"); }
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
				if ( c == '\t' ) { charleft &= ~(8-1); }
			}
		}
		if ( fd != stdinfd ) { close(fd); }
	}
	return result;
}
