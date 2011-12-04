#include <libmaxsi/platform.h>
#include <libmaxsi/io.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/keyboard.h>
#include <libmaxsi/string.h>
#include <libmaxsi/sortix-vga.h>
#include <libmaxsi/sortix-keyboard.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>

using namespace Maxsi;
using namespace Maxsi::Keyboard;

const int width = 80;
const int height = 25;

const int rowstride = width + 2;
const int buffersize = (height+2) * (width+2);

int vgafd;
uint16_t frame[width*height];

bool running;
int posx;
int posy;

char framea[buffersize];
char frameb[buffersize];
char* currentframe;
char* lastframe;

void Clear()
{
	// Reset the game data.
	for ( int i = 0; i < buffersize; i++ ) { framea[i] = 0; frameb[i] = 0; }
}

bool FlushVGA()
{
	return writeall(vgafd, frame, sizeof(frame)) == 0;
}

int Init()
{
	vgafd = open("/dev/vga", O_RDWR);
	if ( vgafd < 0 ) { error(0, errno, "unable to open vga device /dev/vga"); return 1; }

	Clear();

	// Initialize variables.
	currentframe = framea;
	lastframe = frameb;

	running = false;
	posy = height / 2 + 1;
	posx = width / 2 + 1;

	return 0;
}

void Cycle()
{
	// Run a cycle of the game of life.
	for ( int y = 1; y <= height; y++ )
	{
		for ( int x = 1; x <= width; x++ )
		{
			int alivearound = 0;
			int current = lastframe[y * rowstride + x];

			if ( lastframe[(y-1) * rowstride + (x-1)] > 0 ) { alivearound++; }
			if ( lastframe[(y-1) * rowstride + (x-0)] > 0 ) { alivearound++; }
			if ( lastframe[(y-1) * rowstride + (x+1)] > 0 ) { alivearound++; }
			if ( lastframe[(y-0) * rowstride + (x-1)] > 0 ) { alivearound++; }
			if ( lastframe[(y-0) * rowstride + (x+1)] > 0 ) { alivearound++; }
			if ( lastframe[(y+1) * rowstride + (x-1)] > 0 ) { alivearound++; }
			if ( lastframe[(y+1) * rowstride + (x-0)] > 0 ) { alivearound++; }
			if ( lastframe[(y+1) * rowstride + (x+1)] > 0 ) { alivearound++; }

			currentframe[y * rowstride + x] = ( (current > 0 && (alivearound == 3 || alivearound == 2)) || (current == 0 && alivearound == 3) ) ? 1 : 0;
		}
	}

	// Swap buffers.
	char* tmp = lastframe; lastframe = currentframe; currentframe = tmp;
}

void Render()
{
	uint16_t* dest = frame;

	uint16_t set = 'O' | (COLOR8_LIGHT_GREY << 8);
	uint16_t unset = ' ' | (COLOR8_LIGHT_GREY << 8);

	// Render each cell directly to the VGA framebuffer!
	for ( int y = 1; y <= height; y++ )
	{
		for ( int x = 1; x <= width; x++ )
		{
			dest[(y-1) * width + (x-1)] = (lastframe[y * rowstride + x] > 0) ? set : unset;

			// Render the cursor.
			if ( !running && x == posx && y == posy )
			{
				dest[(y-1) * width + (x-1)] |= (COLOR8_RED << 12);
			}
		}
	}

	FlushVGA();
}

void Update()
{
	// Read the keyboard input from the user.
	unsigned method = System::Keyboard::POLL;
	uint32_t codepoint;
	while ( (codepoint = System::Keyboard::ReceiveKeystroke(method) ) != 0 )
	{
		bool keyup = codepoint & DEPRESSED;
		if ( keyup ) { continue; }
		codepoint &= ~DEPRESSED;

		if ( codepoint == 'r' || codepoint == 'R' ) { running = !running; }

		if ( !running )
		{
			if ( codepoint == 'c' || codepoint == 'C' ) { Clear(); }
			if ( codepoint == 'w' || codepoint == 'W' ) { if ( posy > 1 ) { posy--; } }
			if ( codepoint == 's' || codepoint == 'S' ) { if ( posy < height ) { posy++; } }
			if ( codepoint == 'a' || codepoint == 'A' ) { if ( posx > 1 ) { posx--; } }
			if ( codepoint == 'd' || codepoint == 'D' ) { if ( posx < width ) { posx++; } }
			if ( codepoint == ' ' ) { lastframe[posy * rowstride + posx] = 1 - lastframe[posy * rowstride + posx]; }
		}
	}

	// Run a cycle.
	if ( running ) { Cycle(); }

	Render();
}

int usage(int argc, char* argv[])
{
	printf("usage: %s [OPTIONS]\n", argv[0]);
	printf("Options:\n");
	printf("  --speed <miliseconds>    How many miliseconds between updates\n");
	printf("  --usage                  Display this screen\n");
	printf("  --help                   Display this screen\n");

	return 0;
}

int main(int argc, char* argv[])
{
	int sleepms = 50;
	for ( int i = 1; i < argc; i++ )
	{
		if ( String::Compare(argv[i], "--help") == 0 ) { return usage(argc, argv); }
		if ( String::Compare(argv[i], "--usage") == 0 ) { return usage(argc, argv); }
		if ( String::Compare(argv[i], "--speed") == 0 && 1 < argc-i )
		{
			sleepms = String::ToInt(argv[++i]);
		}
	}

	int result = Init();
	if ( result != 0 ) { return result; }

	// Update the game every 50th milisecond.
	while ( true )
	{
		Thread::USleep(sleepms * 1000);
		Update();
	}

	return 0;
}
