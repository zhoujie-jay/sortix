#include <libmaxsi/platform.h>
#include <libmaxsi/io.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/keyboard.h>
#include <libmaxsi/sortix-vga.h>
#include <libmaxsi/sortix-keyboard.h>

using namespace Maxsi;
using namespace Maxsi::Keyboard;

const int width = 80;
const int height = 25;

const int rowstride = width + 2;
const int buffersize = (height+2) * (width+2);

System::VGA::Frame* frame;

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
	// Create a VGA frame we can render onto.
	frame = System::VGA::CreateFrame();
	if ( frame == NULL )
	{
		Print("Could not create VGA frame\n");
		return -1;
	}

	System::VGA::ChangeFrame(frame->fd);

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
	uint16_t* dest = frame->text;

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
	unsigned method = System::Keyboard::POLL;
	uint32_t codepoint;
	while ( (codepoint = System::Keyboard::ReceieveKeystroke(method) ) != 0 )
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

int main(int argc, char* argv[])
{
	int result = Init();
	if ( result != 0 ) { return result; }

	// Update the game every 50th milisecond.
	while ( true )
	{
		const int sleepms = 50;
		Thread::USleep(sleepms * 1000);
		Update();
	}

	return 0;
}
