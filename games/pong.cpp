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

	pong.cpp
	Two-player pong.

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

int Init();
void Reset();
void ClearScreen();
void Collision();
void Goal(unsigned player);
void UpdateUI();
void Update();
void ReadInput();
int main(int argc, char* argv[]);

const int width = 80;
const int height = 25;
const int padsize = 5;

const unsigned goalfreq = 800;
const unsigned collisionfreq = 1200;

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

int Init()
{
	Reset();
	return 0;
}

void Reset()
{
	ClearScreen();

	ballx = width/2;
	bally = height/2;
	ballvelx = -1;
	ballvely = -1;
	oldballx = ballx - ballvelx;
	oldbally = bally - ballvely;
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
}

void Collision()
{
	//System::Sound::SetFrequency(collisionfreq);
	soundleft = 40;
}

void Goal(unsigned player)
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

	//System::Sound::SetFrequency(goalfreq);
	soundleft = 50;

	UpdateUI();
}

void UpdateUI()
{
	for ( int x = 0; x < width; x++ ) { frame[x] = ' ' | (COLOR8_LIGHT_GREY << 12) | (COLOR8_RED << 8); }

	char num[12];
	int len;

	len = sprintf(num, "%u", p1score);
	for ( int i = 0; i < len; i++ ) { frame[i] = ( frame[i] & 0xFF00 ) | num[i]; }

	len = sprintf(num, "%u", p2score);
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
	unsigned termmode = TERMMODE_KBKEY
	                  | TERMMODE_UNICODE
	                  | TERMMODE_SIGNAL
	                  | TERMMODE_NONBLOCK;
	if ( settermmode(0, termmode) ) { error(1, errno, "settermmode"); }
	while ( true )
	{
		uint32_t codepoint;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { return; }
		if ( numbytes < 0 ) { return; }
		int kbkey = KBKEY_DECODE(codepoint);
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
		if ( kbkey == KBKEY_ENTER ) { Reset(); }
		if ( abskbkey == KBKEY_W ) { p1vup = (0 < kbkey); }
		if ( abskbkey == KBKEY_S ) { p1vdown = (0 < kbkey); }
		if ( abskbkey == KBKEY_UP ) { p2vup = (0 < kbkey); }
		if ( abskbkey == KBKEY_DOWN ) { p2vdown = (0 < kbkey); }
	}
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

	while (true)
	{
		usleep(sleepms * 1000);
		ReadInput();
		Update();
		UpdateUI();
		struct dispd_framebuffer* fb = dispd_begin_render(window);
		if ( !fb )
			error(1, 0, "unable to begin rendering dispd window");
		memcpy(dispd_get_framebuffer_data(fb), frame, sizeof(frame));
		dispd_finish_render(fb);

		if ( /*soundleft < 0*/ false ) { continue; }
		if ( soundleft <= 50 )
		{
			//System::Sound::SetFrequency(0);
		}
		else
		{
			soundleft -= sleepms;
		}
	}

	return 0;
}

