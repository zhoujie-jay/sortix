#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/sortix-keyboard.h>

const int MODE_QUIT = 0;
const int MODE_TEXT = 1;
const int MODE_CONFIRM_QUIT = 2;
const int MODE_SAVE = 3;
const int MODE_LOAD = 4;

const unsigned WIDTH = 80;
const unsigned HEIGHT = 24;
char buffers[HEIGHT+1][WIDTH+1];

unsigned cursorx = 0;
unsigned cursory = 0;
unsigned numlines = 1;

char filename[256];

bool bufferchanged;

void clearbuffers()
{
	for ( unsigned y = 0; y < HEIGHT + 1; y++ )
	{
		for ( unsigned x = 0; x < WIDTH + 1; x++ ) { buffers[y][x] = 0; }
	}

	bufferchanged = true;
}

using namespace Maxsi;

void cursorto(unsigned x, unsigned y)
{
	printf("\e[%u;%uH", y+1+1, x+1);
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

	cursorto(cursorx, cursory);
}

unsigned textmode()
{
	drawtextmode();

	bool ctrl = false;

	while ( true )
	{
		cursorto(cursorx, cursory);

		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Keyboard::DEPRESSED )
		{
			if ( codepoint == Keyboard::CTRL | Keyboard::DEPRESSED ) { ctrl = false; }
			continue;
		}

		if ( ctrl )
		{
			if ( codepoint == 'o' || codepoint == 'O' ) { return MODE_SAVE; }
			if ( codepoint == 'r' || codepoint == 'R' ) { return MODE_LOAD; }
			if ( codepoint == 'x' || codepoint == 'X' ) { return MODE_CONFIRM_QUIT; }
			continue;
		}

		switch ( codepoint )
		{
			case Keyboard::ESC:
				return MODE_CONFIRM_QUIT;
				break;
			case Keyboard::CTRL:
				ctrl = true;
				break;
			case Keyboard::UP:
				if ( cursory ) { cursory--; }
				break;
			case '\n':
				cursorx = 0;
				if ( cursory < HEIGHT-1 ) { numlines++; }
			case Keyboard::DOWN:
				if ( cursory < numlines-1 ) { cursory++; }
				break;
			case Keyboard::LEFT:
				if ( cursorx ) { cursorx--; }
				break;
			case Keyboard::RIGHT:
				if ( cursorx < WIDTH-1 ) { cursorx++; }
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

	while ( true )
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Keyboard::DEPRESSED ) { continue; }

		switch ( codepoint )
		{
			case Keyboard::ESC:
				return MODE_QUIT;
				break;
			case 'n':
			case 'N':
				return MODE_TEXT;
			case 'y':
			case 'Y':
				return MODE_QUIT;
			default:
				printf("Would you like to quit? N for No, Y for Yes\n");
		}
	}
}

bool writeall(int fd, const void* buffer, size_t len)
{
	const char* buf = (const char*) buffer;
	while ( len )
	{
		ssize_t byteswritten = write(fd, buf, len);
		if ( byteswritten < 0 ) { return false; }
		buf += byteswritten;
		len -= byteswritten;
	}

	return true;
}

bool savetofile(const char* path)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if ( fd < 0 ) { printf("%s: %s\n", path, strerror(errno)); return false; }

	for ( unsigned y = 0; y < numlines; y++ )
	{
		size_t len = strlen(buffers[y]);
		buffers[y][len] = '\n';
		bool result = writeall(fd, buffers[y], len+1);
		buffers[y][len] = 0;
		if ( !result ) { printf("%s: %s\n", path, strerror(errno)); close(fd); return false; }
	}

	if ( close(fd) ) { printf("%s: %s\n", path, strerror(errno)); return false; }
	return true;
}

int savemode()
{
	printf("\e37m\e40m\e[2J\e[H");
	printf("Please enter the filename you wish to save the text to and press "
	       "enter. Type an empty filename or press ESC to abort.\n\n");

	char writefilename[256];
	strcpy(writefilename, filename);

retry:
	size_t len = strlen(writefilename);
	printf("File Name to Write: %s", writefilename);

	bool readytosave = false;

	while ( !readytosave )
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Keyboard::DEPRESSED ) { continue; }

		switch ( codepoint )
		{
			case Keyboard::ESC:
				return MODE_TEXT;
				break;
			case '\b':
				if ( 0 < len ) { printf("\b"); writefilename[--len] = 0; }
				break;
			case '\n':
				if ( len == 0 ) { return MODE_TEXT; }
				readytosave = true;
				printf("\n");
				break;
			default:
				if ( 0x80 <= codepoint ) { continue; }
				writefilename[len++] = codepoint;
				writefilename[len+1] = 0;
				char msg[2];
				msg[0] = codepoint;
				msg[1] = 0;
				printf("%s", msg);
		}
	}

	if ( !savetofile(writefilename) ) { goto retry; }

	strcpy(filename, writefilename);
	bufferchanged = false;

	printf("Succesfully saved\n");
	sleep(1);
	return MODE_TEXT;
}

bool loadfromfile(const char* path)
{
	int fd = open(path, O_RDONLY, 0777);
	if ( fd < 0 ) { printf("%s: %s\n", path, strerror(errno)); return false; }

	clearbuffers();

	const size_t BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];

	bool done = false;
	while ( !done )
	{
		ssize_t bytesread = read(fd, buffer, BUFFER_SIZE);
		if ( bytesread < 0 ) { printf("%s: %s\n", path, strerror(errno)); close(fd); return false; }
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

	return true;
}

int loadmode()
{
	printf("\e37m\e40m\e[2J\e[H");
	printf("Please enter the filename you wish to load text from and press "
	       "enter. Type an empty filename or press ESC to abort.\n\n");

	char loadfilename[256];
	loadfilename[0] = 0;

retry:
	size_t len = strlen(loadfilename);
	printf("File Name to Load: %s", loadfilename);

	bool readytoload = false;

	while ( !readytoload )
	{
		unsigned method = System::Keyboard::POLL;
		uint32_t codepoint = System::Keyboard::ReceiveKeystroke(method);

		if ( codepoint == 0 ) { continue; }
		if ( codepoint & Keyboard::DEPRESSED ) { continue; }

		switch ( codepoint )
		{
			case Keyboard::ESC:
				return MODE_TEXT;
				break;
			case '\b':
				if ( 0 < len ) { printf("\b"); loadfilename[--len] = 0; }
				break;
			case '\n':
				if ( len == 0 ) { return MODE_TEXT; }
				readytoload = true;
				printf("\n");
				break;
			default:
				if ( 0x80 <= codepoint ) { continue; }
				loadfilename[len++] = codepoint;
				loadfilename[len+1] = 0;
				char msg[2];
				msg[0] = codepoint;
				msg[1] = 0;
				printf("%s", msg);
		}
	}

	if ( !loadfromfile(loadfilename) )
	{
		clearbuffers();
		cursorx = 0;
		cursory = 0;
		goto retry;
	}

	strcpy(filename, loadfilename);
	bufferchanged = false;

	return MODE_TEXT;
}

void run()
{
	bufferchanged = false;

	unsigned mode = MODE_TEXT;
	while ( mode != MODE_QUIT )
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
}

int main(int argc, char* argv[])
{
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
