#include <libmaxsi/platform.h>
#include <libmaxsi/io.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/keyboard.h>
#include <libmaxsi/sortix-vga.h>
#include <libmaxsi/sortix-keyboard.h>
#include <libmaxsi/sortix-sound.h>

using namespace Maxsi;
using namespace Maxsi::Keyboard;

int Init();
void Reset();
void ClearScreen();
void Collision();
void Goal(nat player);
void UpdateUI();
void Update();
void ReadInput();
int main(int argc, char* argv[]);

const int width = 80;
const int height = 25;
const int padsize = 5;

const unsigned goalfreq = 800;
const unsigned collisionfreq = 1200;

System::VGA::Frame* frame;

int ballx;
int bally;
int oldballx;
int oldbally;
int ballvelx;
int ballvely;
int p1y;
int p2y;
bool p1vup;
bool p1vdown;
bool p2vup;
bool p2vdown;
unsigned p1score;
unsigned p2score;
unsigned time;
unsigned soundleft;

int Init()
{
	frame = System::VGA::CreateFrame();
	if ( frame == NULL )
	{
		Print("Could not create VGA frame\n");
		return -1;
	}

	System::VGA::ChangeFrame(frame->fd);

	Reset();

	return 0;
}

void Reset()
{
	ClearScreen();

	ballx = width/2;
	bally = height/2;
	oldballx = ballx;
	oldbally = bally;
	ballvelx = -1;
	ballvely = -1;
	p1y = (height-padsize) / 5;
	p2y = (height-padsize) / 5;
	p1vup = false;
	p1vdown = false;
	p2vup = false;
	p2vdown = false;
	p1score = 0;
	p2score = 0;
	time = 0;
	soundleft = -1;
}

void ClearScreen()
{
	for ( int y = 0; y < height; y++ )
	{
		uint16_t color = COLOR8_BLACK << 8;

		for ( int x = 0; x < width; x++ )
		{
			frame->text[x + y*width] = ' ' | color;
		}
	}
}

void Collision()
{
	System::Sound::SetFrequency(collisionfreq);
	soundleft = 40;
}

void Goal(nat player)
{
	frame->text[ballx + bally*width] = ' ' | (COLOR8_WHITE << 8);

	ballx = width/2;
	bally = height/2;

	if ( player == 1 )
	{
		ballvelx = 1;
		ballvely = 1;
		p1score++;
	}
	else if ( player == 2 )
	{
		ballvelx = -1;
		ballvely = -1;
		p2score++;
	}

	System::Sound::SetFrequency(goalfreq);
	soundleft = 50;

	UpdateUI();	
}

void UpdateUI()
{
	for ( int x = 0; x < width; x++ ) { frame->text[x] = ' ' | (COLOR8_LIGHT_GREY << 12) | (COLOR8_RED << 8); }
}

void Update()
{
	int p1v = 0, p2v = 0;

	if ( p1vup && !p1vdown ) { p1v = -1; } else if ( !p1vup && p1vdown ) { p1v = 1; }
	if ( p2vup && !p2vdown ) { p2v = -1; } else if ( !p2vup && p2vdown ) { p2v = 1; }

	if ( p1v < 0 && p1y > 1 ) { p1y--; }
	if ( p1v > 0 && p1y + padsize < height ) { p1y++; }
	if ( p2v < 0 && p2y > 1 ) { p2y--; }
	if ( p2v > 0 && p2y + padsize < height ) { p2y++; }

	for ( int y = 1; y < height; y++ )
	{
		uint16_t color = ( y < p1y || y >= p1y + padsize ) ? COLOR8_BLACK << 12 : COLOR8_RED << 12; frame->text[y*width] = ' ' | color;
	}

	for ( int y = 1; y < height; y++ )
	{
		uint16_t color = ( y < p2y || y >= p2y + padsize ) ? COLOR8_BLACK << 12 : COLOR8_BLUE << 12; frame->text[width-1 + y*width] = ' ' | color;
	}

	if ( bally + ballvely <= 1 ) { ballvely = 0 - ballvely; Collision(); }
	if ( bally + ballvely >= height ) { ballvely = 0 - ballvely; Collision(); }

	if ( ballx + ballvelx < 1 ) { if ( bally + ballvely < p1y - 1 || bally + ballvely > p1y + padsize + 1 ) { Goal(2); } else { ballvelx = 0 - ballvelx; Collision(); } }
	if ( ballx + ballvelx >= width-1 ) { if ( bally + ballvely < p2y - 1 || bally + ballvely > p2y + padsize + 1 ) { Goal(1); } else { ballvelx = 0 - ballvelx; Collision(); } }

	frame->text[oldballx + oldbally*width] = ' ' | (COLOR8_WHITE << 8);
	frame->text[ballx + bally*width] = '.' | (COLOR8_WHITE << 8);
	oldballx = ballx; oldbally = bally;

	ballx += ballvelx;
	bally += ballvely;

	frame->text[ballx + bally*width] = 'o' | (COLOR8_WHITE << 8);
}

void ReadInput()
{
	unsigned method = System::Keyboard::POLL;
	uint32_t codepoint;
	while ( (codepoint = System::Keyboard::ReceieveKeystroke(method) ) != 0 )
	{
		bool keyup = codepoint & DEPRESSED;
		codepoint &= ~DEPRESSED;

		if ( codepoint == '\n' && keyup ) { Reset(); }
		if ( codepoint == 'w' || codepoint == 'W' ) { p1vup = !keyup; }
		if ( codepoint == 's' || codepoint == 'S' ) { p1vdown = !keyup; }
		if ( codepoint == UP ) { p2vup = !keyup; }
		if ( codepoint == DOWN ) { p2vdown = !keyup; }
	}
}

int main(int argc, char* argv[])
{
	int result = Init();
	if ( result != 0 ) { return result; }

	while (true)
	{
		ReadInput();
		Update();
		UpdateUI();
		const unsigned sleepms = 50;
		Thread::USleep(sleepms * 1000);
		if ( soundleft < 0 ) { continue; }
		if ( soundleft <= 50 )
		{
			System::Sound::SetFrequency(0);
		}
		else
		{
			soundleft -= sleepms;
		}
	}

	return 0;
}

