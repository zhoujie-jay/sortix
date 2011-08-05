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

	log.cpp
	A system for logging various messages to the kernel log.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "globals.h"
#include "iprintable.h"
#include "log.h"

#ifdef PLATFORM_SERIAL
#include "uart.h"
#endif

using namespace Maxsi;

namespace Sortix
{
#ifdef OLD_LOG
	DEFINE_GLOBAL_OBJECT(Log, Log);

	void Log::Init()
	{
#ifdef PLATFORM_SERIAL
		// Set the cursor to (0,0)
		const char InitMessage[] = { String::ASCII_ESCAPE, '[', 'H' };
		UART::Write(InitMessage, sizeof(InitMessage));

		return;
#endif

		Line = 0;
		Column = 0;

		// Empty the screen.
		size_t* Msgs = (size_t*) Messages;
		size_t Msg = ' ';
#ifdef PLATFORM_X86
		Msg |= Msg << 8; Msg |= Msg << 16;
#elif defined(PLATFORM_X64)
		Msg |= Msg << 8; Msg |= Msg << 16; Msg |= Msg << 32;
#endif
		for ( nat I = 0; I < (Height*Width) / sizeof(size_t); I++ ) { Msgs[I] = Msg; }

		// Set the color for each line.
		for ( nat Y = 0; Y < Height; Y++ ) { Colors[Y] = (COLOR8_LIGHT_GREY << 0) + (COLOR8_BLACK << 4); }

		// Flush to our viewing device.
		UpdateScreen();
	}

	void Log::UpdateScreen()
	{
#ifdef PLATFORM_SERIAL
		return;
#endif

		uint16_t* Destination = (uint16_t*) 0xB8000;

		// Copy and apply colors.
		for ( nat Y = 0; Y < Height; Y++ )
		{
			uint16_t Color = ((uint16_t) Colors[Y]) << 8;

			for ( nat X = 0; X < Width; X++ )
			{
				Destination[X + Y*Width] = ( (uint16_t) Messages[X + Y*Width] ) | Color;
			}
		}

		SetCursor(Column, Line);
	}

#ifndef PLATFORM_SERIAL
	size_t Log::Print(const char* Message)
	{
		size_t Written;
		while ( *Message != '\0' /*&& *Message != 0*/ )
		{
			if ( *Message == '\n' )
			{
				NewLine();
			}
			else if ( *Message == '\t' )
			{
				if ( 80 <= Column ) { NewLine(); }
				do { Messages[Column + Line * Width] = ' '; Column++; } while ( Column % 4 != 0 );
			}
			else if ( *Message == '\r' )
			{
				// No need for this driver to support line resets.
			}
			else if ( *Message == '\b' )
			{
				// No need for this driver to support backspaces.
			}
			else
			{
				if ( 80 <= Column ) { NewLine(); }
				Messages[Column + Line * Width] = *Message; Column++;
			}

			Message++;
			Written++;
		}
		
		return Written;
	}
#else
	size_t Log::Print(const char* Message)
	{
		size_t Written;
		while ( *Message != '\0' /*&& *Message != 0*/ )
		{
			if ( *Message == '\n' )
			{
				UART::WriteChar('\r');
				UART::WriteChar(*Message);
			}
			//else if ( *Message == '\t' )
			//{
			//	if ( 80 <= Column ) { NewLine(); }
			//	do { Messages[Column + Line * Width] = ' '; Column++; } while ( Column % 4 != 0 );
			//}
			else
			{
				UART::WriteChar(*Message);
			}

			Message++;
			Written++;
		}
		
		return Written;
	}
#endif

	void Log::NewLine()
	{
#ifdef PLATFORM_SERIAL
		return;
#endif

		if ( Line < Height - 1 )
		{
			Line++;
		}
		else
		{
			// Scroll down our buffer.
			Memory::Copy(Messages, Messages + Width, (Height-1)*Width);

			// Scroll down our color buffer as well.
			for ( nat Y = 1; Y < Height; Y++ ) { Colors[Y-1] = Colors[Y]; }
		}

		// Reset the new line.
		Memory::Set(Messages + Line*Width, ' ', Width);

		// Assume the color of the new line is the same as the previous.
		Colors[Line] = Colors[Line-1];

		Column = 0;

		// Flush our output.
		UpdateScreen();
	}

	void Log::SetCursor(nat X, nat Y)
	{
#ifdef PLATFORM_SERIAL
		return;
#endif

		nat Value = X + Y * Width;

		// This sends a command to indicies 14 and 15 in the
		// CRT Control Register of the VGA controller. These
		// are the high and low bytes of the index that show
		// where the hardware cursor is to be 'blinking'.
		CPU::OutPortB(0x3D4, 14);
		CPU::OutPortB(0x3D5, (Value >> 8) & 0xFF);
		CPU::OutPortB(0x3D4, 15);
		CPU::OutPortB(0x3D5, (Value >> 0) & 0xFF);
	}


#else
	namespace Log
	{
		Maxsi::Format::Callback deviceCallback = NULL;
		void* devicePointer = NULL;

		void Init(Maxsi::Format::Callback callback, void* user)
		{
			deviceCallback = callback;
			devicePointer = user;
		}
	}

#endif
}
