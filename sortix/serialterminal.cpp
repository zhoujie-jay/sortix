/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    serialterminal.cpp
    A terminal on a serial line.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/keyboard.h>
#include <string.h>
#include "vga.h"
#include "uart.h"
#include "serialterminal.h"
#include "scheduler.h"

namespace Sortix
{
	namespace SerialTerminal
	{
		void Reset()
		{
			// Set the cursor to (0,0) and clear the screen.
			const char InitMessage[] = "\e[37m\e[40m\e[2J\e[H";
			UART::Write(InitMessage, strlen(InitMessage));
		}

		bool isEsc;
		bool isEscDepress;
		int sigpending;

		const bool ECHO_TO_VGA = true;

		bool cursordisabled;

		void Init()
		{
			Reset();

			isEsc = isEscDepress = false;
			sigpending = -1;
			cursordisabled = false;
		}

		void OnTick()
		{
			// TODO: Yeah, this is a bad hack.
			int c;
			while ( (c=UART::TryPopChar()) != -1 )
			{
				// TODO: Support for hooking the serial input up against the keyboard API have broken
				#if 0
				// TODO: This is no longer compatible with the keyboard API, so
				// it has been commented out. Besides, JSSortix isn't really
				// supported anyway. It'd be nice to refactor this into a
				// SerialKeyboard class or something.

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
				if ( c == 145 ) // Control Depressed
				{
					if ( sigpending < 0 ) { continue; }
					if ( sigpending == 'C' - 'A' + 1 )
					{
						Scheduler::SigIntHack();
						continue;
					}

					Keyboard::QueueKeystroke(CTRL);
					Keyboard::QueueKeystroke('A' + sigpending - 1);
					Keyboard::QueueKeystroke(CTRL | DEPRESSED);
					sigpending = -1;
					continue;
				}
				if ( c < 32 ) { sigpending = c; } else { sigpending = -1; }
				switch ( c )
				{
					default:
						// Ignore most unprintable characters.
						if ( c < 32 ) { continue; }
					case '\b':
					case '\t':
					case '\r':
					case '\n':
					case '\e':
					case '\f':
						break;
				}
				if ( c == 127 ) { c = '\b'; }
				if ( c & (1<<7) )
				{
					c &= ~(1<<7); c |= DEPRESSED;
				}
				Keyboard::QueueKeystroke(c);
				#endif
			}
		}

		void OnVGAModified()
		{
			if ( !cursordisabled )
			{
				const char* msg = "\e[l";
				UART::Write(msg, strlen(msg));
				cursordisabled = true;
			}
			UART::RenderVGA((const uint16_t*) 0xB8000);
		}

		size_t Print(void* /*user*/, const char* string, size_t stringlen)
		{
			// TODO: Echoing to the VGA terminal is broken
#if 0
			if ( ECHO_TO_VGA ) { VGATerminal::Print(NULL, string, stringlen); }
#endif
			if ( cursordisabled )
			{
				const char* msg = "\e[h";
				UART::Write(msg, strlen(msg));
				cursordisabled = false;
			}
			UART::Write(string, stringlen);
			return stringlen;
		}
	}
}
