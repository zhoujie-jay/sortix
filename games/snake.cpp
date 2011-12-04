#include <libmaxsi/platform.h>
#include <libmaxsi/io.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/keyboard.h>
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
	return writeall(vgafd, frame, sizeof(frame)) == 0;
}

int Init()
{
	vgafd = open("/dev/vga", O_RDWR);
	if ( vgafd < 0 ) { error(0, errno, "unable to open vga device /dev/vga"); return 1; }

	Reset();

	return 0;
}

void Update()
{
	int newvelx = velx;
	int newvely = vely;

	// Read the keyboard input from the user.
	unsigned method = System::Keyboard::POLL;
	uint32_t codepoint;
	while ( (codepoint = System::Keyboard::ReceiveKeystroke(method) ) != 0 )
	{
		bool keyup = codepoint & DEPRESSED;
		if ( keyup ) { continue; }
		codepoint &= ~DEPRESSED;

		if ( codepoint == '\n' ) { Reset(); return; }
		if ( codepoint == 'w' || codepoint == 'W' ) { newvelx = 0; newvely = -1; }
		if ( codepoint == 'a' || codepoint == 'A' ) { newvelx = -1; newvely = 0; }
		if ( codepoint == 's' || codepoint == 'S' ) { newvelx = 0; newvely = 1; }
		if ( codepoint == 'd' || codepoint == 'D' ) { newvelx = 1; newvely = 0; }
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
		ASSERT(0 <= animalx && animalx < width);
		ASSERT(0 <= animaly && animaly < height);
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
	int result = Init();
	if ( result != 0 ) { return result; }

	// Update the game every once in a while.
	while ( true )
	{
		Thread::USleep(speed * 1000);
		Update();
		FlushVGA();
	}

	return 0;
}
