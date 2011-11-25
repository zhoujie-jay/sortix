#include <libmaxsi/platform.h>
#include <libmaxsi/io.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/keyboard.h>
#include <libmaxsi/string.h>
#include <libmaxsi/sortix-vga.h>
#include <libmaxsi/sortix-keyboard.h>
#include <libmaxsi/sortix-sound.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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

int vgafd;
uint16_t frame[width*height];

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

bool FlushVGA()
{
	return writeall(vgafd, frame, sizeof(frame)) == 0;
}

int Init()
{
	vgafd = open("/dev/vga", O_RDWR);
	if ( vgafd < 0 ) { printf("Unable to open vga device: %s", strerror(errno)); return 1; }

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
			frame[x + y*width] = ' ' | color;
		}
	}

	FlushVGA();
}

void Collision()
{
	System::Sound::SetFrequency(collisionfreq);
	soundleft = 40;
}

void Goal(nat player)
{
	frame[ballx + bally*width] = ' ' | (COLOR8_WHITE << 8);

	int offset = (rand() % 4) - 2;
	ballx = width/2;
	bally = height/2 + offset;

	if ( player == 1 )
	{
		ballvelx = 1;
		ballvely = (rand() % 2) * 2 - 1;
		p1score++;
	}
	else if ( player == 2 )
	{
		ballvelx = -1;
		ballvely = (rand() % 2) * 2 - 1;
		p2score++;
	}

	if ( ballvely > 0 ) { ballvely /= ballvely; }
	if ( ballvely < 0 ) { ballvely /= -ballvely; }

	System::Sound::SetFrequency(goalfreq);
	soundleft = 50;

	UpdateUI();	
}

void UpdateUI()
{
	for ( int x = 0; x < width; x++ ) { frame[x] = ' ' | (COLOR8_LIGHT_GREY << 12) | (COLOR8_RED << 8); }

	char num[12];
	int len;

	len = String::ConvertUInt32(p1score, num);
	for ( int i = 0; i < len; i++ ) { frame[i] = ( frame[i] & 0xFF00 ) | num[i]; }

	len = String::ConvertUInt32(p2score, num);
	for ( int i = 0; i < len; i++ ) { frame[width - len + i] = ( frame[width - len + i] & 0xFF00 ) | num[i]; }
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
		uint16_t color = ( y < p1y || y >= p1y + padsize ) ? COLOR8_BLACK << 12 : COLOR8_RED << 12; frame[y*width] = ' ' | color;
	}

	for ( int y = 1; y < height; y++ )
	{
		uint16_t color = ( y < p2y || y >= p2y + padsize ) ? COLOR8_BLACK << 12 : COLOR8_BLUE << 12; frame[width-1 + y*width] = ' ' | color;
	}

	if ( bally + ballvely <= 1 ) { ballvely = 0 - ballvely; Collision(); }
	if ( bally + ballvely >= height ) { ballvely = 0 - ballvely; Collision(); }

	if ( ballx + ballvelx < 1 ) { if ( bally + ballvely < p1y - 1 || bally + ballvely > p1y + padsize + 1 ) { Goal(2); } else { ballvelx = 0 - ballvelx; Collision(); } }
	if ( ballx + ballvelx >= width-1 ) { if ( bally + ballvely < p2y - 1 || bally + ballvely > p2y + padsize + 1 ) { Goal(1); } else { ballvelx = 0 - ballvelx; Collision(); } }

	frame[oldballx + oldbally*width] = ' ' | (COLOR8_WHITE << 8);
	frame[ballx + bally*width] = '.' | (COLOR8_WHITE << 8);
	oldballx = ballx; oldbally = bally;

	ballx += ballvelx;
	bally += ballvely;

	frame[ballx + bally*width] = 'o' | (COLOR8_WHITE << 8);
}

void ReadInput()
{
	unsigned method = System::Keyboard::POLL;
	uint32_t codepoint;
	while ( (codepoint = System::Keyboard::ReceiveKeystroke(method) ) != 0 )
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

	while (true)
	{
		ReadInput();
		Update();
		UpdateUI();
		FlushVGA();
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

