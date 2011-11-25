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

	serialterminal.cpp
	A terminal on a serial line.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/keyboard.h>
#include "log.h"
#include "vga.h"
#include "keyboard.h"
#include "uart.h"
#include "serialterminal.h"
#include "vgaterminal.h"
#include "scheduler.h"

namespace Sortix
{
	namespace SerialTerminal
	{
		void Reset()
		{
			// Set the cursor to (0,0) and clear the screen.
			const char InitMessage[] = "\e[37m\e[40m\e[2J\e[H";
			UART::Write(InitMessage, Maxsi::String::Length(InitMessage));
		}

		size_t numvgaframes = 0;

		bool isEsc;
		bool isEscDepress;

		void Init()
		{
			numvgaframes = 0;
			Reset();

			isEsc = isEscDepress = false;
		}

		void OnTick()
		{
			// TODO: Yeah, this is a bad hack.
			int c;
			while ( (c=UART::TryPopChar()) != -1 )
			{
				using namespace Maxsi::Keyboard;

				if ( !isEsc && c == '\e' ) { isEsc = true; continue; }
				if ( isEsc && c == '\e' ) { isEsc = false; }
				if ( isEsc && c == '[' ) { continue; }
				if ( isEsc && c == ']' ) { isEscDepress = true; continue; }
				if ( isEsc && !isEscDepress && c == 'A' ) { Keyboard::QueueKeystroke(UP); }
				if ( isEsc && !isEscDepress && c == 'B' ) { Keyboard::QueueKeystroke(DOWN); }
				if ( isEsc && !isEscDepress && c == 'C' ) { Keyboard::QueueKeystroke(RIGHT); }
				if ( isEsc && !isEscDepress && c == 'D' ) { Keyboard::QueueKeystroke(LEFT); }
				if ( isEsc && isEscDepress && c == 'A' ) { Keyboard::QueueKeystroke(UP | DEPRESSED); }
				if ( isEsc && isEscDepress && c == 'B' ) { Keyboard::QueueKeystroke(DOWN | DEPRESSED); }
				if ( isEsc && isEscDepress && c == 'C' ) { Keyboard::QueueKeystroke(RIGHT | DEPRESSED); }
				if ( isEsc && isEscDepress && c == 'D' ) { Keyboard::QueueKeystroke(LEFT | DEPRESSED); }
				if ( isEsc ) { isEsc = false; isEscDepress = false; continue; }
				if ( c == '\e' ) { c = ESC; }
				if ( c == ('\e' | (1<<7)) ) { c = ESC | DEPRESSED; }
				if ( c == 'c' - 'a'+1 ) { Scheduler::SigIntHack(); continue; }
				if ( c == 'o' - 'a'+1 ) { Keyboard::QueueKeystroke(CTRL); Keyboard::QueueKeystroke('O');  Keyboard::QueueKeystroke(CTRL | DEPRESSED); continue; }
				if ( c == 'r' - 'a'+1 ) { Keyboard::QueueKeystroke(CTRL); Keyboard::QueueKeystroke('R');  Keyboard::QueueKeystroke(CTRL | DEPRESSED); continue; }
				if ( c == 'x' - 'a'+1 ) { Keyboard::QueueKeystroke(CTRL); Keyboard::QueueKeystroke('X');  Keyboard::QueueKeystroke(CTRL | DEPRESSED); continue; }
				if ( c == 127 ) { c = '\b'; }
				if ( c & (1<<7) )
				{
					c &= ~(1<<7); c |= DEPRESSED;
				}
				Keyboard::QueueKeystroke(c);
			}

#ifdef PLATFORM_SERIAL
			// TODO: But this hack may be worse.
			if ( numvgaframes )
			{
				UART::RenderVGA((const uint16_t*) 0xB8000);
			}
#endif
		}

		void OnVGAFrameCreated()
		{
			if ( numvgaframes++ == 0 )
			{
				UART::WriteChar('\e');
				UART::WriteChar('[');
				UART::WriteChar('l');
			}
		}

		void OnVGAFrameDeleted()
		{
			if ( --numvgaframes == 0 )
			{
				Reset();
				UART::WriteChar('\e');
				UART::WriteChar('[');
				UART::WriteChar('h');
			}
		}

		size_t Print(void* /*user*/, const char* string, size_t stringlen)
		{
			if ( numvgaframes )
			{
				VGATerminal::Print(NULL, string, stringlen);
#ifdef PLATFORM_SERIAL
				UART::RenderVGA((const uint16_t*) 0xB8000);
#endif
			}
			else
			{
				UART::Write(string, stringlen);
			}

			return stringlen;
		}
	}
}
