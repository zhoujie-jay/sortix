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

	shell.cpp
	A very basic command line shell.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include "globals.h"
#include "iprintable.h"
#include "iirqhandler.h"
#include "log.h"
#include "keyboard.h"
#include "shell.h"

using namespace Maxsi;

namespace Sortix
{
	DEFINE_GLOBAL_OBJECT(Shell, Shell);

	const char* Prefix = "root@sortix / # ";

	void Shell::Init()
	{
		BacklogUsed = 0;
		Column = 0;

		// Empty the screen.
#ifdef PLATFORM_X86
		size_t Msg = ' '; Msg |= Msg << 8; Msg |= Msg << 16;
#elif defined(PLATFORM_X64)
		size_t Msg = ' '; Msg |= Msg << 8; Msg |= Msg << 16; Msg |= Msg << 32;
#endif
		for ( nat I = 0; I < (BacklogHeight*Width) / sizeof(size_t); I++ ) { Backlog[I] = Msg; }
		for ( nat I = 0; I < (Height*Width) / sizeof(size_t); I++ ) { Command[I] = Msg; }

		// Flush to our viewing device.
		UpdateScreen();
	}

	void Shell::ClearScreen()
	{
		uint16_t* Destination = (uint16_t*) 0xB8000;

		for ( size_t I = 0; I < (Height*Width)/sizeof(size_t); I++ ) { Destination[I] = 0; }
	}

	void Shell::UpdateScreen()
	{
		ClearScreen();

		uint16_t* Destination = (uint16_t*) 0xB8000;

		uint16_t Color = ((COLOR8_LIGHT_GREY << 0) + (COLOR8_BLACK << 4)) << 8;

		nat CommandLines = (Column + String::Length(Prefix)) / Width + 1;

		nat BacklogLines = Height - CommandLines; if ( BacklogUsed < BacklogLines ) { BacklogLines = BacklogUsed; }
		for ( nat I = 0; I < BacklogLines; I++ )
		{
			nat Line = BacklogUsed - BacklogLines + I;

			for ( nat X = 0; X < Width; X++ ) { Destination[X + I*Width] = ( (uint16_t) Backlog[X + Line*Width] ) | Color; }
		}

		uint16_t* Pos = Destination + Width * BacklogLines;

		size_t PrefixLen = String::Length(Prefix);
		for ( size_t I = 0; I < PrefixLen; I++ ) { Pos[I] = ( (uint16_t) Prefix[I] ) | Color; }
		Pos += PrefixLen;
		for ( size_t I = 0; I < Column; I++ ) {	Pos[I] = ( (uint16_t) Command[I] ) | Color; }

		SetCursor((PrefixLen + Column) % Width, BacklogLines + CommandLines - 1);
	}

	void Shell::OnKeystroke(uint32_t CodePoint, bool KeyUp)
	{
		if ( CodePoint == '\n' )
		{
			//Execute();
		}
		else if ( CodePoint == '\t' )
		{
			// No autocompletion yet!
		}
		else if ( CodePoint == '\r' )
		{
			// No need for this driver to support line resets.
		}
		else if ( CodePoint == '\b' )
		{
			if ( 0 < Column ) { Column--; }
		}
		else
		{
			if ( CodePoint <= 0xFF )
			{
				Command[Column] = CodePoint; Column++;
			}
		}

		UpdateScreen();
	}

#if 0
	void Shell::NewLine()
	{
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
#endif

	void Shell::SetCursor(nat X, nat Y)
	{
		nat Value = X + Y * Width;

		// This sends a command to indicies 14 and 15 in the
		// CRT Control Register of the VGA controller. These
		// are the high and low bytes of the index that show
		// where the hardware cursor is to be 'blinking'.
		X86::OutPortB(0x3D4, 14);
		X86::OutPortB(0x3D5, (Value >> 8) & 0xFF);
		X86::OutPortB(0x3D4, 15);
		X86::OutPortB(0x3D5, (Value >> 0) & 0xFF);
	}
}
