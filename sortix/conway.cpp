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

	conway.cpp
	THE GAME! You lost it. OF LIFE!

******************************************************************************/

#include "platform.h"

#include "globals.h"
#include "iprintable.h"
#include "log.h"
#include "vga.h"

#ifdef PLATFORM_SERIAL
#include "uart.h"
#endif

namespace Sortix
{
	namespace Conway
	{
		const int Height = 25;
		const int Width = 80;
		const int RowStride = Width + 2;
		const int BufferSize = (Height+2) * (Width+2);

#ifdef PLATFORM_SERIAL
		VGA::Frame FrameData;
		VGA::Frame* Frame = &FrameData;
#else
		VGA::Frame* Frame = (VGA::Frame*) 0xB8000;
#endif

		char FrameA[BufferSize];
		char FrameB[BufferSize];
		char* CurrentFrame;
		char* LastFrame;
		bool Running;
		int PosX;
		int PosY;
		int TimeToNextFrame;
		int Time;

		void Clear()
		{
			for ( int I = 0; I < BufferSize; I++ ) { FrameA[I] = 0; FrameB[I] = 0; }
		}
	
		void Init()
		{
			CurrentFrame = FrameA;
			LastFrame = FrameB;

			Running = false;

			PosY = Height / 2 + 1;
			PosX = Width / 2 + 1;
			Time = 0;

			Clear();
		}

		void Cycle()
		{
			for ( int Y = 1; Y <= Height; Y++ )
			{
				for ( int X = 1; X <= Width; X++ )
				{
					int AliveAround = 0;
					int Current = LastFrame[Y * RowStride + X];

					if ( LastFrame[(Y-1) * RowStride + (X-1)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y-1) * RowStride + (X-0)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y-1) * RowStride + (X+1)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y-0) * RowStride + (X-1)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y-0) * RowStride + (X+1)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y+1) * RowStride + (X-1)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y+1) * RowStride + (X-0)] > 0 ) { AliveAround++; }
					if ( LastFrame[(Y+1) * RowStride + (X+1)] > 0 ) { AliveAround++; }

					CurrentFrame[Y * RowStride + X] = ( (Current > 0 && (AliveAround == 3 || AliveAround == 2)) || (Current == 0 && AliveAround == 3) ) ? 1 : 0;
				}
			}

			// Swap buffers.
			char* TMP = LastFrame; LastFrame = CurrentFrame; CurrentFrame = TMP;
		}

		void Render()
		{
			uint16_t* Destination = Frame->Data;

			uint16_t Set = 'O' | (COLOR8_LIGHT_GREY << 8);
			uint16_t Unset = ' ' | (COLOR8_LIGHT_GREY << 8);

			for ( int Y = 1; Y <= Height; Y++ )
			{
				for ( int X = 1; X <= Width; X++ )
				{
					Destination[(Y-1) * Width + (X-1)] = (LastFrame[Y * RowStride + X] > 0) ? Set : Unset;
				}
			}

			// Render the cursor.
			if ( !Running )
			{
				Destination[(PosY-1) * Width + (PosX-1)] |= (COLOR8_RED << 12);
			}

#ifdef PLATFORM_SERIAL
			UART::RenderVGA(Frame);
#endif
		}

		void Update(int TimePassed)
		{
#ifdef PLATFORM_SERIAL
			while ( true )
			{
				int C = UART::TryPopChar();

				if ( C == 'r' || C == 'R' ) { TimeToNextFrame = 0; Running = !Running; }

				if ( !Running )
				{
					if ( C == 'w' || C == 'W' ) { if ( PosY > 1 ) { PosY--; } }
					if ( C == 's' || C == 'S' ) { if ( PosY < Height ) { PosY++; } }
					if ( C == 'a' || C == 'A' ) { if ( PosX > 1 ) { PosX--; } }
					if ( C == 'd' || C == 'D' ) { if ( PosX < Width ) { PosX++; } }
					if ( C == 'c' || C == 'C' ) { Clear(); }
					if ( C == ' ' ) { LastFrame[PosY * RowStride + PosX] = 1 - LastFrame[PosY * RowStride + PosX]; }
				}
			
				if ( C == -1 ) { break; }
			}
#endif

			if ( Running )
			{
				if ( TimeToNextFrame <= TimePassed )
				{
					Cycle();
					TimeToNextFrame = 50;
				}
				else
				{
					TimeToNextFrame -= TimePassed;
				}
			}

			Render();
		}

		void OnFuture()
		{
			const int TimePassed = 10;

			Time += TimePassed;


#ifdef PLATFORM_SERIAL

#ifdef JSSORIX
			const nat WaitTime = 2000;
#else
			const nat WaitTime = 3500;
#endif

			// Artificially display the boot welcome screen.
			if ( Time < WaitTime ) { return; }
#endif

			Update(TimePassed);
		}
	}
}
