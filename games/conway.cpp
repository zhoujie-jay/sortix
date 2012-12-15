/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	conway.cpp
	Conway's Game of Life.

*******************************************************************************/

#include <sortix/vga.h>
#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>

#include <dispd.h>

const int width = 80;
const int height = 25;

const int rowstride = width + 2;
const int buffersize = (height+2) * (width+2);

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

int Init()
{
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
}

void Update()
{
	// Read the keyboard input from the user.
	unsigned termmode = TERMMODE_KBKEY
	                  | TERMMODE_UNICODE
	                  | TERMMODE_SIGNAL
	                  | TERMMODE_NONBLOCK;
	if ( settermmode(0, termmode) ) { error(1, errno, "settermmode"); }
	while ( true )
	{
		uint32_t codepoint;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { break; }
		if ( numbytes < 0 ) { break; }
		int kbkey = KBKEY_DECODE(codepoint);
		if ( kbkey == KBKEY_R ) { running = !running; }
		if ( running ) { continue; }
		if ( kbkey == KBKEY_C ) { Clear(); }
		if ( kbkey == KBKEY_W ) { if ( posy > 1 ) { posy--; } }
		if ( kbkey == KBKEY_A ) { if ( posx > 1 ) { posx--; } }
		if ( kbkey == KBKEY_S ) { if ( posy < height ) { posy++; } }
		if ( kbkey == KBKEY_D ) { if ( posx < width ) { posx++; } }
		if ( kbkey == KBKEY_SPACE )
		{
			lastframe[posy * rowstride + posx] = 1 - lastframe[posy * rowstride + posx];
		}
	}

	// Run a cycle.
	if ( running ) { Cycle(); }

	Render();
}

int usage(int /*argc*/, char* argv[])
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
	if ( !dispd_initialize(&argc, &argv) )
		error(1, 0, "couldn't initialize dispd library");

	int sleepms = 50;
	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp(argv[i], "--help") == 0 ) { return usage(argc, argv); }
		if ( strcmp(argv[i], "--usage") == 0 ) { return usage(argc, argv); }
		if ( strcmp(argv[i], "--speed") == 0 && 1 < argc-i )
		{
			sleepms = atoi(argv[++i]);
		}
	}

	int result = Init();
	if ( result != 0 ) { return result; }

	struct dispd_session* session = dispd_attach_default_session();
	if ( !session )
		error(1, 0, "couldn't attach to dispd default session");
	if ( !dispd_session_setup_game_vga(session) )
		error(1, 0, "couldn't setup dispd vga session");
	struct dispd_window* window = dispd_create_window_game_vga(session);
	if ( !window )
		error(1, 0, "couldn't create dispd vga window");

	// Update the game every 50th milisecond.
	while ( true )
	{
		usleep(sleepms * 1000);
		Update();
		struct dispd_framebuffer* fb = dispd_begin_render(window);
		if ( !fb )
			error(1, 0, "unable to begin rendering dispd window");
		memcpy(dispd_get_framebuffer_data(fb), frame, sizeof(frame));
		dispd_finish_render(fb);
	}

	return 0;
}
