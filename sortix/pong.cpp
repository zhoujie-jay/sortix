/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along

	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	pong.cpp
	ROBOT PONG ATTACK!

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include "globals.h"
#include "iirqhandler.h"
#include "iprintable.h"
#include "keyboard.h"
#include "time.h"
#include "vga.h"
#include "sound.h"
#include "pong.h"

#ifdef JSSORTIX
#include "uart.h"
#endif

using namespace Maxsi;

namespace Sortix
{
	namespace Pong
	{
		const int Height = 25;
		const int Width = 80;
		const int PadSize = 5;

		nat Time;
		nat NextUpdate;
		bool InIntro;

		int P1Y;
		int P2Y;
		bool P1VUP;
		bool P2VUP;
		bool P1VDOWN;
		bool P2VDOWN;
		nat P1Score;
		nat P2Score;
		nat HardCore;

		int BallX;
		int BallY;
		int VelX;
		int VelY;
		int OldBallX;
		int OldBallY;

		int SoundLeft;

		const nat GoalFreq = 800;
		const nat CollisionFreq = 1200;

		void Init()
		{
			ClearScreen();

			P1Y = (Height-PadSize) / 5;
			P2Y = (Height-PadSize) / 5;
			P1VUP = false;
			P2VUP = false;
			P1VDOWN = false;
			P2VDOWN = false;

			Time = 0;

			SoundLeft = -1;

			Reset();
#ifndef JSSORTIX
			Intro();
#endif
		}

		void Intro()
		{
			InIntro = true;

			ClearScreen();

			uint16_t* Destination = (uint16_t*) 0xB8000;

			const char* String = "THIS IS SORTIX PONG"; size_t StringLen = String::Length(String);
			const char* Warn = "SortixV3-beta Edition"; size_t WarnLen = String::Length(Warn);
			const char* Enter = "-- press enter to continue --"; size_t EnterLen = String::Length(Enter);

			// Copy and apply colors.
			uint16_t Color = COLOR8_GREEN << 8;

			for ( size_t I = 0; I < StringLen; I++ ) { Destination[Width / 2 - StringLen / 2 + I + (Height/2)*Width] = (uint16_t)(String[I]) | Color; }
			for ( size_t I = 0; I < WarnLen; I++ ) { Destination[Width / 2 - WarnLen / 2 + I + (Height/2 + 1)*Width] = (uint16_t)(Warn[I]) | Color; }
			for ( size_t I = 0; I < EnterLen; I++ ) { Destination[Width / 2 - EnterLen / 2 + I + (Height/2 + 2)*Width] = (uint16_t)(Enter[I]) | Color; }

		}

		void ClearScreen()
		{
			uint16_t* Destination = (uint16_t*) 0xB8000;

			// Copy and apply colors.
			for ( int Y = 0; Y < Height; Y++ )
			{
				uint16_t Color = COLOR8_BLACK << 8;

				for ( int X = 0; X < Width; X++ )
				{
					Destination[X + Y*Width] = ' ' | Color;
				}
			}
		}

		void Reset()
		{
			P1Score = 0;
			P2Score = 0;
			InIntro = false;
			BallX = Width/2;
			BallY = Height/2;
			OldBallX = BallX;
			OldBallY = BallY;
			VelX = -1;
			VelY = -1;
			HardCore = 60;
			UpdateUI();
		}

		void Collision()
		{
			Sound::Play(CollisionFreq);
			SoundLeft = 40;
		}

		void Goal(nat Player)
		{
			uint16_t* Destination = (uint16_t*) 0xB8000;
			Destination[BallX + BallY*Width] = ' ' | (COLOR8_WHITE << 8);

			BallX = Width/2;
			BallY = Height/2;

			if ( Player == 1 )
			{
				VelX = 1;
				VelY = 1;
				P1Score++;
			}
			else if ( Player == 2 )
			{
				VelX = -1;
				VelY = -1;
				P2Score++;
			}

			Sound::Play(GoalFreq);
			SoundLeft = 50;

			//HardCore -= 2;

			UpdateUI();	
		}

		void UpdateUI()
		{
			uint16_t* Destination = (uint16_t*) 0xB8000;

			for ( int X = 0; X < Width; X++ ) { Destination[X] = ' ' | (COLOR8_LIGHT_GREY << 12) | (COLOR8_RED << 8); }
	
			char Num[12];
			int Len;

			Len = UInt32ToString(P1Score, Num);
			for ( int I = 0; I < Len; I++ ) { Destination[I] = ( Destination[I] & 0xFF00 ) | Num[I]; }

			Len = UInt32ToString(P2Score, Num);
			for ( int I = 0; I < Len; I++ ) { Destination[Width - Len + I] = ( Destination[Width - Len + I] & 0xFF00 ) | Num[I]; }
		}

		void Update()
		{
#ifdef JSSORTIX

			while ( true )
			{
				int C = UART::TryPopChar();

				if ( C == 'w' || C == 'W' ) { P1VUP = true; P1VDOWN = false; }
				if ( C == 's' || C == 'S' ) { P1VDOWN = true; P1VUP = false; }
				if ( C == 'o' || C == 'O' ) { P2VUP = true; P1VDOWN = false; }
				if ( C == 'l' || C == 'L' ) { P2VDOWN = true; P2VUP = false;  }

				if ( C == -1 ) { break; }
			}
#endif

			uint16_t* Destination = (uint16_t*) 0xB8000;

			int P1V = 0, P2V = 0;

			if ( P1VUP && !P1VDOWN ) { P1V = -1; } else if ( !P1VUP && P1VDOWN ) { P1V = 1; }
			if ( P2VUP && !P2VDOWN ) { P2V = -1; } else if ( !P2VUP && P2VDOWN ) { P2V = 1; }

			if ( P1V < 0 && P1Y > 1 ) { P1Y--; }
			if ( P1V > 0 && P1Y + PadSize < Height ) { P1Y++; }
			if ( P2V < 0 && P2Y > 1 ) { P2Y--; }
			if ( P2V > 0 && P2Y + PadSize < Height ) { P2Y++; }

			for ( int Y = 1; Y < Height; Y++ )
			{
				uint16_t Color = ( Y < P1Y || Y >= P1Y + PadSize ) ? COLOR8_BLACK << 12 : COLOR8_RED << 12; Destination[Y*Width] = ' ' | Color;
			}

			for ( int Y = 1; Y < Height; Y++ )
			{
				uint16_t Color = ( Y < P2Y || Y >= P2Y + PadSize ) ? COLOR8_BLACK << 12 : COLOR8_BLUE << 12; Destination[Width-1 + Y*Width] = ' ' | Color;
			}

			if ( BallY + VelY <= 1 ) { VelY = 0 - VelY; Collision(); }
			if ( BallY + VelY >= Height ) { VelY = 0 - VelY; Collision(); }

			if ( BallX + VelX < 1 ) { if ( BallY + VelY < P1Y - 1 || BallY + VelY > P1Y + PadSize + 1 ) { Goal(2); } else { VelX = 0 - VelX; Collision(); } }
			if ( BallX + VelX >= Width-1 ) { if ( BallY + VelY < P2Y - 1 || BallY + VelY > P2Y + PadSize + 1 ) { Goal(1); } else { VelX = 0 - VelX; Collision(); } }

			Destination[OldBallX + OldBallY*Width] = ' ' | (COLOR8_WHITE << 8);
			Destination[BallX + BallY*Width] = '.' | (COLOR8_WHITE << 8);
			OldBallX = BallX; OldBallY = BallY;

			BallX += VelX;
			BallY += VelY;

			Destination[BallX + BallY*Width] = 'o' | (COLOR8_WHITE << 8);

#ifdef JSSORTIX
			UART::RenderVGA();

			P1VUP = P1VDOWN = P2VUP = P2VDOWN = false; 
#endif

			NextUpdate = Time + HardCore;
		}

		void OnKeystroke(uint32_t CodePoint, bool KeyUp)
		{
			const uint32_t UP = 0xFFFFFFFF - 20;
			const uint32_t DOWN = 0xFFFFFFFF - 23;

			if ( CodePoint == '\n' )
			{
				ClearScreen();
				Reset();
				Update();
			}

			if ( CodePoint == 'w' || CodePoint == 'W' ) { P1VUP = !KeyUp; }
			if ( CodePoint == 's' || CodePoint == 'S' ) { P1VDOWN = !KeyUp; }
			if ( CodePoint == UP ) { P2VUP = !KeyUp; }
			if ( CodePoint == DOWN ) { P2VDOWN = !KeyUp; }
		}

		void OnFuture()
		{
			const int TimePassed = 10;

			Time += TimePassed;

#ifdef JSSORTIX
			// Artificially display the boot welcome screen.
			if ( Time < 2000 ) { return; }
#endif

			if ( SoundLeft > 0 )
			{
				if ( SoundLeft <= TimePassed )
				{
					Sound::Mute();
					SoundLeft = -1;
				}
				else
				{
					SoundLeft -= TimePassed;
				}
			}

			if ( InIntro ) { return; }

			if ( NextUpdate <= Time ) { Update(); }
		}
	}
}
