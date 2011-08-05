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

	astroids.cpp
	IN SPACE, NO ONE CAN HEAR YOU find ~ | xargs shred

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include "globals.h"
#include "iirqhandler.h"
#include "iprintable.h"
#include "keyboard.h"
#include "timer.h"
#include "log.h"
#include "astroids.h"

using namespace Maxsi;

namespace Sortix
{
	DEFINE_GLOBAL_OBJECT(Astroids, Astroids);

	const int Height = 25;
	const int Width = 80;

	void Astroids::Init()
	{
		Time = 0;

		Reset();
		Update();

		GKeyboard->SetReader(this);
		GTimer->SetTimerable(this);
	}

	void Astroids::ClearScreen()
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

	void Astroids::Reset()
	{
		ClearScreen();
		PX = Width/2;
		PY = Height/2;
		PVX = 0;
		PVY = 0;
		TrailX = PX;
		TrailY = PY;
		TrailLen = 0;
		TrailMaxLen = 15;
	}

	void Astroids::Update()
	{
		uint16_t* Destination = (uint16_t*) 0xB8000;

		PX += PVX; PX = ((nat) PX) % ((nat) Width);
		PY += PVY; PY = ((nat) PY) % ((nat) Height);

		uint16_t Prev = 0;
		if ( !(PVX == 0 && PVY == 0) && Destination[PX + PY*Width] > 0 ) { Reset(); }
		
		if ( !(PVX == 0 && PVY == 0) )	
		{
			if ( PVX < 0 ) { Prev = 0; }
			if ( PVX > 0 ) { Prev = 1; }
			if ( PVY < 0 ) { Prev = 2; }
			if ( PVY > 0 ) { Prev = 3; }			

			if ( TrailMaxLen <= TrailLen )
			{
				uint16_t AtTrail = Destination[TrailX + TrailY*Width];
				Destination[TrailX + TrailY*Width] = ' ' | (COLOR8_BLACK << 8);
				if ( (AtTrail & 0x4) == 0 ) { TrailX--; } else
				if ( (AtTrail & 0x4) == 1 ) { TrailX++; } else
				if ( (AtTrail & 0x4) == 2 ) { TrailY--; } else
				if ( (AtTrail & 0x4) == 3 ) { TrailY++; }
			}
			else { TrailLen++; }
		}
		Destination[PX + PY*Width] = (COLOR8_GREEN << 12) | Prev;

		NextUpdate = Time + 75;
	}

	void Astroids::OnKeystroke(uint32_t CodePoint, bool KeyUp)
	{
		const uint32_t UP = 0xFFFFFFFF - 20;
		const uint32_t LEFT = 0xFFFFFFFF - 21;
		const uint32_t RIGHT = 0xFFFFFFFF - 22;
		const uint32_t DOWN = 0xFFFFFFFF - 23;

		if ( CodePoint == '\n' && !KeyUp )
		{
			ClearScreen();
			Reset();
			Update();
		}

		if ( !(PVX == 0 && PVY == 1) && (CodePoint == 'w' || CodePoint == 'W') && !KeyUp ) { PVY = -1; PVX = 0; }
		if ( !(PVX == 1 && PVY == 0) && (CodePoint == 'a' || CodePoint == 'A') && !KeyUp ) { PVY = 0; PVX = -1; }
		if ( !(PVX == 0 && PVY == -1) && (CodePoint == 's' || CodePoint == 'S') && !KeyUp ) { PVY = 1; PVX = 0; }
		if ( !(PVX == -1 && PVY == 0) && (CodePoint == 'd' || CodePoint == 'D') && !KeyUp ) { PVY = 0; PVX = 1; }
	}

	void Astroids::OnFuture(uint32_t Miliseconds)
	{
		Time += Miliseconds;

		if ( NextUpdate <= Time ) { Update(); }
	}
}
