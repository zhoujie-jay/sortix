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

	snake.cpp
	Single-player snake.

*******************************************************************************/

#include <sortix/vga.h>
#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>

const int width = 80;
const int height = 25;

const int buffersize = height * width;

int vgafd;
uint16_t frame[width*height];

int posx;
int posy;
int velx;
int vely;
int tailx;
int taily;
int taillen;
int tailmax;
int animalx;
int animaly;

char direction[buffersize];

uint16_t animal = '%' | (COLOR8_RED<<8);
uint16_t snake = ' ' | (COLOR8_GREEN<<12);

const int defaultspeed = 75;
const int speedincrease = -5;
const int maxspeed = 40;
volatile int speed;

bool tabhack = false;

void Clear()
{
	// Reset the game data.
	for ( int i = 0; i < buffersize; i++ ) { frame[i] = ' '; direction[i] = -1; }
}

void Reset()
{
	Clear();
	tailx = posx = width/2;
	taily = posy = height/2;
	switch ( rand() % 4 )
	{
		case 0: velx = -1; vely = 0; break;
		case 1: velx = 1; vely = 0; break;
		case 2: velx = 0; vely = -1; break;
		case 3: velx = 0; vely = -1; break;
	}

	animalx = 2 + (rand() % (width-4));
	animaly = 2 + (rand() % (height-4));

	taillen = 0;
	tailmax = 3;

	frame[animaly * width + animalx] = animal;

	speed = defaultspeed;
}

bool FlushVGA()
{
	return writeall(vgafd, frame, sizeof(frame)) < sizeof(frame);
}

int Init()
{
	bool has_vga_mode_set = true;
	FILE* modefp = fopen("/dev/video/mode", "r");
	if ( modefp )
	{
		char* mode = NULL;
		size_t modesize;
		if ( 0 <= getline(&mode, &modesize, modefp) )
		{
			if ( strcmp(mode, "driver=none\n") != 0 )
				has_vga_mode_set = false;
			free(mode);
		}
		fclose(modefp);
	}

	if ( !has_vga_mode_set )
	{
		fprintf(stderr, "Sorry, this game only works in VGA mode.\n");
		return 1;
	}

	vgafd = open("/dev/vga", O_RDWR);
	if ( vgafd < 0 ) { error(0, errno, "unable to open vga device /dev/vga"); return 1; }

	Reset();

	return 0;
}

bool tabhacking = false;

void Update()
{
	int newvelx = velx;
	int newvely = vely;

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
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
		if ( tabhack && abskbkey == KBKEY_TAB ) { tabhacking = (0 < kbkey); }
		if ( kbkey == KBKEY_ENTER ) { Reset(); return; }
		if ( kbkey == KBKEY_W ) { newvelx = 0; newvely = -1; }
		if ( kbkey == KBKEY_A ) { newvelx = -1; newvely = 0; }
		if ( kbkey == KBKEY_S ) { newvelx = 0; newvely = 1; }
		if ( kbkey == KBKEY_D ) { newvelx = 1; newvely = 0; }
	}

	if ( tabhack && tabhacking )
	{
		if ( animalx == posx && posy < animaly ) { newvelx = 0; newvely = 1; }
		if ( animalx == posx && posy > animaly ) { newvelx = 0; newvely = -1; }
		if ( animaly == posy && posx < animalx ) { newvelx = 1; newvely = 0; }
		if ( animaly == posy && posx > animalx ) { newvelx = -1; newvely = 0; }
	}

	// Make sure we don't do a 180.
	if ( !(velx == -1 && newvelx == 1) &&
	     !(velx == 1 && newvelx == -1) &&
	     !(vely == -1 && newvely == 1) &&
	     !(vely == 1 && newvely == -1) )
	{
		velx = newvelx; vely = newvely;
	}

	// Don't collide with the border!
	if ( posx + velx < 0 || width <= posx + velx ||
	     posy + vely < 0 || height <= posy + vely ) { Reset(); return; }

	int newx = posx + velx;
	int newy = posy + vely;

	// Move the tail, if needed.
	if ( taillen == tailmax )
	{
		frame[taily * width + tailx] = ' '; taillen--;
		switch ( direction[taily * width + tailx] )
		{
			case 0: tailx--; break;
			case 1: tailx++; break;
			case 2: taily--; break;
			case 3: taily++; break;
		}
	}

	// Check for collision.
	if ( frame[newy * width + newx] == snake ) { Reset(); return; }

	// Check for food.
	if ( newx == animalx && newy == animaly )
	{
		tailmax++;
		animalx = 2 + (rand() % (width-4));
		animaly = 2 + (rand() % (height-4));
		assert(0 <= animalx && animalx < width);
		assert(0 <= animaly && animaly < height);
		if ( maxspeed < speed ) { speed += speedincrease; }
	}

	frame[animaly * width + animalx] = animal;

	// Remember where we are going.
	int dir = 0;
	if ( velx < 0 ) { dir = 0; }
	if ( velx > 0 ) { dir = 1; }
	if ( vely < 0 ) { dir = 2; }
	if ( vely > 0 ) { dir = 3; }
	direction[posy * width + posx] = dir;

	// Move the head.
	posx = newx;
	posy = newy;
	frame[posy * width + posx] = snake; taillen++;
}

int main(int argc, char* argv[])
{
	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp("--tabhack", argv[i]) == 0 ) { tabhack = true; }
	}

	int result = Init();
	if ( result != 0 ) { return result; }

	// Update the game every once in a while.
	while ( true )
	{
		usleep(speed * 1000);
		Update();
		FlushVGA();
	}

	return 0;
}
