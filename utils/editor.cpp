/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    editor.cpp
    A simple and hacked together text editor.

*******************************************************************************/

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <termios.h>

const int MODE_QUIT = 1;
const int MODE_TEXT = 2;
const int MODE_CONFIRM_QUIT = 3;
const int MODE_SAVE = 4;
const int MODE_LOAD = 5;

const unsigned WIDTH = 80;
const unsigned HEIGHT = 24;
char buffers[HEIGHT+1][WIDTH+1];

unsigned cursorx = 0;
unsigned cursory = 0;
unsigned numlines = 1;

char filename[256];

bool bufferchanged = false;

void clearbuffers()
{
	for ( unsigned y = 0; y < HEIGHT + 1; y++ )
	{
		for ( unsigned x = 0; x < WIDTH + 1; x++ ) { buffers[y][x] = 0; }
	}

	bufferchanged = true;
}

void cursorto(unsigned x, unsigned y)
{
	printf("\e[%u;%uH", y+1+1, x+1);
	fflush(stdout);
}

char* readline(int fd)
{
	unsigned oldtermmode;
	if ( gettermmode(fd, &oldtermmode) ) { return NULL; }

	unsigned termmode = TERMMODE_UNICODE
	                  | TERMMODE_SIGNAL
	                  | TERMMODE_UTF8
	                  | TERMMODE_LINEBUFFER
	                  | TERMMODE_ECHO;
	if ( settermmode(fd, termmode) ) { return NULL; }

	size_t lineused = 0;
	size_t linelength = 32UL;
	char* line = new char[linelength + 1];
	line[0] = '\0';

	while ( true )
	{
		char c;
		ssize_t numbytes = read(fd, &c, sizeof(c));
		if ( numbytes < 0 ) { delete[] line; line = NULL; break; }
		if ( !numbytes ) { break; }
		if ( c == '\n' ) { break; }

		if ( lineused == linelength )
		{
			size_t newlinelength = 2 * linelength;
			char* newline = new char[newlinelength];
			if ( !newline ) { delete[] line; line = NULL; break; }
			memcpy(newline, line, lineused * sizeof(*line));
			delete[] line;
			line = newline;
			linelength = newlinelength;
		}

		line[lineused++] = c;
		line[lineused] = '\0';
	}

	if ( settermmode(fd, oldtermmode) ) { delete[] line; line = NULL; }
	return line;
}

void drawtextmode()
{
	const char* printfilename = ( strlen(filename) > 0 ) ? filename : "New Buffer";
	printf("\e[30m\e[47m\e[2J\e[H");
	printf("Text Editor\t\tfile: %s\n", printfilename);
	printf("\e[37m\e[40m\e[0J");
	for ( unsigned y = 0; y < HEIGHT; y++ )
	{
		printf("%s", buffers[y]);
		if ( y < HEIGHT-1 ) { printf("\n"); }
	}

	fflush(stdout);
	cursorto(cursorx, cursory);
}

unsigned textmode()
{
	drawtextmode();

	bool ctrl = false;
	unsigned dectrlmode = 0;

	int oldcursorx = -1;
	int oldcursory = -1;
	while ( true )
	{
		if ( oldcursorx != (int) cursorx || oldcursory != (int) cursory )
		{
			cursorto(cursorx, cursory);
			oldcursorx = cursorx;
			oldcursory = cursory;
		}

		unsigned termmode = TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL;
		if ( settermmode(0, termmode) ) { error(1, errno, "settermmode"); }

		uint32_t codepoint = 0;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { break; }
		if ( numbytes < 0 ) { error(1, errno, "read stdin"); }
		if ( numbytes < (ssize_t) sizeof(codepoint) ) {
			printf("unexpectedly got %zi bytes\n", numbytes);
			printf("bytes: %x\n", codepoint);


fprintf(stderr, "bad stdin data\n"); exit(1); }
		if ( !codepoint ) { continue; }
		int kbkey = KBKEY_DECODE(codepoint);
		if ( kbkey )
		{
			if ( kbkey == -KBKEY_LCTRL && dectrlmode ) { return dectrlmode; }
			int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
			if ( abskbkey == KBKEY_LCTRL ) { ctrl = (0 < kbkey); continue; }
			switch ( kbkey )
			{
			case -KBKEY_ESC:
				return MODE_CONFIRM_QUIT;
				break;
			case KBKEY_UP:
				if ( cursory ) { cursory--; }
				break;
			case KBKEY_DOWN:
				if ( cursory < numlines-1 ) { cursory++; }
				break;
			case KBKEY_LEFT:
				if ( cursorx ) { cursorx--; }
				break;
			case KBKEY_RIGHT:
				if ( cursorx < WIDTH-1 ) { cursorx++; }
				break;
			case KBKEY_O:
				if ( ctrl ) { dectrlmode = MODE_SAVE; }
				break;
			case KBKEY_R:
				if ( ctrl ) { dectrlmode = MODE_LOAD; }
				break;
			case KBKEY_X:
				if ( ctrl ) { dectrlmode = MODE_CONFIRM_QUIT; }
				break;
			}
			continue;
		}

		if ( ctrl ) { continue; }

		switch ( codepoint )
		{
			case '\n':
				cursorx = 0;
				if ( cursory < HEIGHT-1 ) { numlines++; cursory++; }
				break;
			case '\b':
				if ( cursorx )
				{
					cursorx--;

					for ( unsigned x = cursorx; x < WIDTH; x++ )
					{
						buffers[cursory][x] = buffers[cursory][x+1];
						bufferchanged = true;
					}
					printf("\e[2K\r%s", buffers[cursory]);
					fflush(stdout);
				}
				else if ( 0 < cursory && strlen(buffers[cursory]) == 0 )
				{
					for ( unsigned y = cursory; y < numlines; y++ )
					{
						for ( unsigned x = 0; x < WIDTH; x++ )
						{
							buffers[y][x] = buffers[y+1][x];
						}
					}

					numlines--;
					cursory--;
					cursorx = strlen(buffers[cursory]);

					drawtextmode();
				}
				break;
		}

		size_t linelen = strlen(buffers[cursory]);
		if ( linelen < cursorx ) { cursorx = linelen; }

		if ( WIDTH <= cursorx ) { continue; }
		if ( codepoint >= 0x80 ) { continue; }
		if ( codepoint == '\t' ) { continue; }
		if ( codepoint == '\b' ) { continue; }
		if ( codepoint == '\n' ) { continue; }
		if ( codepoint == '\r' ) { continue; }

		char msg[2];
		msg[0] = codepoint;
		msg[1] = 0;
		printf("%s", msg);
		buffers[cursory][cursorx++] = codepoint;
		fflush(stdout);
		bufferchanged = true;
		if ( WIDTH <= cursorx ) { cursorx = WIDTH-1; }
	}

	return MODE_QUIT;
}

unsigned confirmquit()
{
	if ( !bufferchanged ) { return MODE_QUIT; }

	printf("\e37m\e40m\e[2J\e[H");
	printf("There are unsaved changes: Are you sure you want to quit? (Y/N)\n");

	if ( settermmode(0, TERMMODE_KBKEY | TERMMODE_SIGNAL | TERMMODE_ECHO) )
	{
		error(1, errno, "settermmode");
	}

	while ( true )
	{
		uint32_t codepoint;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { exit(0); }
		if ( numbytes < 0 ) { error(1, errno, "read stdin"); }
		if ( numbytes < (ssize_t) sizeof(codepoint) ) { fprintf(stderr, "bad stdin data\n"); exit(1); }
		if ( !codepoint ) { continue; }

		int kbkey = KBKEY_DECODE(codepoint);
		if ( !kbkey ) { continue; }
		if ( 0 < kbkey ) { continue; }

		switch ( kbkey )
		{
			case -KBKEY_ESC:
				return MODE_QUIT;
				break;
			case -KBKEY_N:
				return MODE_TEXT;
			case -KBKEY_Y:
				return MODE_QUIT;
			default:
				printf("Would you like to quit? N for No, Y for Yes\n");
		}
	}
}

bool savetofile(const char* path)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if ( fd < 0 ) { error(0, errno, "%s", path); return false; }

	for ( unsigned y = 0; y < numlines; y++ )
	{
		size_t len = strlen(buffers[y]);
		buffers[y][len] = '\n';
		bool result = writeall(fd, buffers[y], len+1) == len+1;
		buffers[y][len] = 0;
		if ( !result ) { error(0, errno, "write: %s", path); close(fd); return false; }
	}

	if ( close(fd) ) { error(0, errno, "close: %s", path); return false; }
	strcpy(filename, path);
	return true;
}

int savemode()
{
	printf("\e37m\e40m\e[2J\e[H");
	printf("Please enter the filename you wish to save the text to and press "
	       "enter. Type an empty filename to abort.\n\n");

	char* storage = NULL;

	do
	{
		delete[] storage;
		printf("File to Write: ");
		fflush(stdout);
		storage = readline(0);
		if ( !storage ) { error(1, errno, "readline"); }
		if ( !storage[0] ) { delete[] storage; return MODE_TEXT; }
	} while ( !savetofile(storage) );
	delete[] storage;

	bufferchanged = false;

	printf("Succesfully saved\n");
	sleep(1);
	return MODE_TEXT;
}

bool loadfromfile(const char* path)
{
	int fd = open(path, O_RDONLY, 0777);
	if ( fd < 0 ) { error(0, errno, "%s", path); return false; }

	clearbuffers();

	const size_t BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	bool done = false;
	while ( !done )
	{
		ssize_t bytesread = read(fd, buffer, BUFFER_SIZE);
		if ( bytesread < 0 ) { error(0, errno, "read: %s", path); close(fd); return false; }
		if ( bytesread == 0 ) { break; }
		for ( ssize_t i = 0; i < bytesread; i++ )
		{
			if ( buffer[i] == '\n' )
			{
				if ( HEIGHT-1 <= ++cursory ) { done = true; break; }
				cursorx = 0;
				continue;
			}

			if ( WIDTH <= cursorx ) { continue; }
			buffers[cursory][cursorx++] = buffer[i];
		}
	}

	numlines = cursory + 1;

	close(fd);

	cursorx = 0;
	cursory = 0;

	strcpy(filename, path);

	return true;
}

int loadmode()
{
	printf("\e37m\e40m\e[2J\e[H");
	printf("Please enter the filename you wish to load text from and press "
	       "enter. Type an empty filename to abort.\n\n");

	char* storage = NULL;

	do
	{
		delete[] storage;
		printf("File to Load: ");
		fflush(stdout);
		storage = readline(0);
		if ( !storage ) { error(1, errno, "readline"); }
		if ( !storage[0] ) { delete[] storage; return MODE_TEXT; }
	} while ( !loadfromfile(storage) );
	delete[] storage;

	bufferchanged = false;

	return MODE_TEXT;
}

void run()
{
	bufferchanged = false;

	unsigned mode = MODE_TEXT;
	while ( mode != (unsigned) MODE_QUIT )
	{
		switch ( mode )
		{
			case MODE_TEXT:
				mode = textmode();
				break;
			case MODE_CONFIRM_QUIT:
				mode = confirmquit();
				break;
			case MODE_SAVE:
				mode = savemode();
				break;
			case MODE_LOAD:
				mode = loadmode();
				break;
			default:
				printf("Application Bug: Unknown Mode\n");
				sleep(1);
				mode = MODE_TEXT;
				break;
		}
	}

	printf("\e[37m\e[40m\e[2J\e[H");
	fflush(stdout);
}

int main(int argc, char* argv[])
{
	if ( !isatty(1) ) { error(1, errno, "stdout must be a tty"); return 1; }
	struct winsize ws;
	if ( tcgetwinsize(1, &ws) != 0 )
		error(1, errno, "tcgetwinsize");
	if ( ws.ws_col != 80 || ws.ws_row != 25 )
	{
		fprintf(stderr, "Sorry, this application only works with 80x25 "
		                "terminal resolutions, please fix it. :)\n");
		exit(1);
	}

	if ( argc < 2 )
	{
		clearbuffers();
		strcpy(filename, "");
	}
	else
	{
		strcpy(filename, argv[1]);
		if ( !loadfromfile(filename) ) { clearbuffers(); }
		cursorx = 0;
		cursory = 0;
	}

	run();

	return 0;
}
