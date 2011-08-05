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

	keyboard.cpp
	A driver for the PS2 Keyboard.

******************************************************************************/

#include "platform.h"
#include "globals.h"
#include "iprintable.h"
#include "iirqhandler.h"
#include "log.h"
#include "panic.h"
#include "keyboard.h"
#include "isr.h"

#include "pong.h"

namespace Sortix
{
	namespace Keyboard
	{
		namespace Layouts
		{
			const uint32_t UNKNOWN = 0xFFFFFFFF;
			const uint32_t ESC = 0xFFFFFFFF - 1;
			const uint32_t CTRL = 0xFFFFFFFF - 2;
			const uint32_t LSHFT = 0xFFFFFFFF - 3;
			const uint32_t RSHFT = 0xFFFFFFFF - 4;
			const uint32_t ALT = 0xFFFFFFFF - 5;
			const uint32_t F1 = 0xFFFFFFFF - 6;
			const uint32_t F2 = 0xFFFFFFFF - 7;
			const uint32_t F3 = 0xFFFFFFFF - 8;
			const uint32_t F4 = 0xFFFFFFFF - 9;
			const uint32_t F5 = 0xFFFFFFFF - 10;
			const uint32_t F6 = 0xFFFFFFFF - 11;
			const uint32_t F7 = 0xFFFFFFFF - 12;
			const uint32_t F8 = 0xFFFFFFFF - 13;
			const uint32_t F9 = 0xFFFFFFFF - 14;
			const uint32_t F10 = 0xFFFFFFFF - 15;
			const uint32_t F11 = 0xFFFFFFFF - 16;
			const uint32_t F12 = 0xFFFFFFFF - 17;
			const uint32_t SCRLCK = 0xFFFFFFFF - 18;
			const uint32_t HOME = 0xFFFFFFFF - 19;
			const uint32_t UP = 0xFFFFFFFF - 20;
			const uint32_t LEFT = 0xFFFFFFFF - 21;
			const uint32_t RIGHT = 0xFFFFFFFF - 22;
			const uint32_t DOWN = 0xFFFFFFFF - 23;
			const uint32_t PGUP = 0xFFFFFFFF - 24;
			const uint32_t PGDOWN = 0xFFFFFFFF - 25;
			const uint32_t END = 0xFFFFFFFF - 26;
			const uint32_t INS = 0xFFFFFFFF - 27;
			const uint32_t DEL = 0xFFFFFFFF - 28;
			const uint32_t CAPS = 0xFFFFFFFF - 29;
			const uint32_t NONE = 0xFFFFFFFF - 30;
			const uint32_t ALTGR = 0xFFFFFFFF - 31;
			const uint32_t NUMLCK = 0xFFFFFFFF - 32;

			namespace US
			{
				uint32_t sg[128] =
				{
					UNKNOWN,
					ESC,
					'1',
					'2',
					'3',
					'4',
					'5',
					'6',
					'7',
					'8',
					'9',
					'0',
					'-',
					'=',
					'\b',
					'\t',
					'q',
					'w',
					'e',
					'r',
					't',
					'y',
					'u',
					'i',
					'o',
					'p',
					'[',
					']',
					'\n',
					CTRL,
					'a',
					's',
					'd',
					'f',
					'g',
					'h',
					'j',
					'k',
					'l',
					';',
					'\'',
					'`',
					LSHFT,
					'\\',
					'z',
					'x',
					'c',
					'v',
					'b',
					'n',
					'm',
					',',
					'.',
					'/',
					RSHFT,
					'*',
					ALT,
					' ',
					CAPS,
					F1,
					F2,
					F3,
					F4,
					F5,
					F6,
					F7,
					F8,
					F9,
					F10,
					NUMLCK,
					SCRLCK,
					HOME,
					UP,
					PGUP,
					'-',
					LEFT,
					UNKNOWN,
					RIGHT,
					'+',
					END,
					DOWN,
					PGDOWN,
					INS,
					DEL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					F11,
					F12,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
				};

				uint32_t Sg[128] =
				{
					UNKNOWN,
					ESC,
					'!',
					'@',
					'#',
					'$',
					'%',
					'^',
					'&',
					'*',
					'(',
					')',
					'_',
					'+',
					'\b',
					'\t', // TODO: Make it de-tab!
					'Q',
					'W',
					'E',
					'R',
					'T',
					'Y',
					'U',
					'I',
					'O',
					'P',
					'{',
					'}',
					'\n',
					CTRL,
					'A',
					'S',
					'D',
					'F',
					'G',
					'H',
					'J',
					'K',
					'L',
					':',
					'"',
					'~',
					LSHFT,
					'|',
					'Z',
					'X',
					'C',
					'V',
					'B',
					'N',
					'M',
					'<',
					'>',
					'?',
					RSHFT,
					'*',
					ALT,
					' ',
					CAPS,
					F1,
					F2,
					F3,
					F4,
					F5,
					F6,
					F7,
					F8,
					F9,
					F10,
					NUMLCK,
					SCRLCK,
					HOME,
					UP,
					PGUP,
					'-',
					LEFT,
					UNKNOWN,
					RIGHT,
					'+',
					END,
					DOWN,
					PGDOWN,
					INS,
					DEL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					F11,
					F12,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
				};

				uint32_t sG[128] =
				{
					UNKNOWN,
					ESC,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					'\b',
					'\t',
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					'\n',
					CTRL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					LSHFT,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					RSHFT,
					UNKNOWN,
					ALT,
					' ',
					CAPS,
					F1,
					F2,
					F3,
					F4,
					F5,
					F6,
					F7,
					F8,
					F9,
					F10,
					NUMLCK,
					SCRLCK,
					HOME,
					UP,
					PGUP,
					'-',
					LEFT,
					UNKNOWN,
					RIGHT,
					'+',
					END,
					DOWN,
					PGDOWN,
					INS,
					DEL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					F11,
					F12,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
				};

				uint32_t SG[128] =
				{
					UNKNOWN,
					ESC,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					'\b',
					'\t',
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					'\n',
					CTRL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					LSHFT,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					RSHFT,
					UNKNOWN,
					ALT,
					' ',
					CAPS,
					F1,
					F2,
					F3,
					F4,
					F5,
					F6,
					F7,
					F8,
					F9,
					F10,
					NUMLCK,
					SCRLCK,
					HOME,
					UP,
					PGUP,
					'-',
					LEFT,
					UNKNOWN,
					RIGHT,
					'+',
					END,
					DOWN,
					PGDOWN,
					INS,
					DEL,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					F11,
					F12,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
					UNKNOWN,
				};

				uint32_t* Layout[4] = { sg, Sg, sG, SG }; 
			}

			uint32_t** Layout = US::Layout;

			nat Mask = 0;
			nat LockMask = 0;

			const nat Shift = (1<<0);
			const nat AltGr = (1<<1);
			const nat ScrLck = (1<<2);
			const nat LEDScrLck = (1<<0);
			const nat LEDNumLck = (1<<1);
			const nat LEDCapsLck = (1<<2);

			uint32_t GetCodePoint(uint8_t Scancode)
			{
				nat TableIndex = 0x3 & (Mask ^ LockMask);

				//Log::PrintF("[m=%x,lm=%x,ti=%x]", Mask, LockMask, TableIndex);

				uint32_t CodePoint = Layout[TableIndex][Scancode & 0x7F];

				if ( CodePoint == UNKNOWN ) { return UNKNOWN; }
				if ( Scancode & 0x80 )
				{
					if ( CodePoint == LSHFT ) { Mask &= ~Shift; }
					if ( CodePoint == ALTGR ) { Mask &= ~AltGr; }
					if ( CodePoint == SCRLCK ) { Mask &= ~ScrLck; }
				}
				else
				{
					if ( CodePoint == LSHFT ) { Mask |= Shift; }
					if ( CodePoint == ALTGR ) { Mask |= AltGr; }
					if ( CodePoint == SCRLCK ) { Mask |= ScrLck; }
					if ( CodePoint == CAPS ) { LockMask ^= Shift; SetLEDs(LEDCapsLck); }
				}

				return CodePoint;
			}
		}

		uint8_t LEDs;

		void Init()
		{
			// Initialize variables.
			LEDs = 0;

			// Register our keystroke callback.
			register_interrupt_handler(IRQ1, OnIRQ1);

			// If any scancodes were already pending, our interrupt handler
			// will never be called. Let's just discard anything pending.
			CPU::InPortB(0x60);
		}

		void OnIRQ1(CPU::InterruptRegisters* Regs)
		{
			// Read the scancode from the data register.
			uint8_t Scancode = CPU::InPortB(0x60);

			//Log::PrintF("[%u]", Scancode);

			uint32_t CodePoint = Layouts::GetCodePoint(Scancode);

			bool KeyUp = (Scancode & 0x80) > 0;

#if PONG
			Pong::OnKeystroke(CodePoint, KeyUp); return;
#endif

#if 1
			return;
#endif
			//if ( Reader != NULL ) { Reader->OnKeystroke(CodePoint, KeyUp); return; }

			// Use this to debug the exact scancodes you receive!
			//Log::PrintF("[%u/U+%x]", Scancode, CodePoint);

			if ( CodePoint == Layouts::UNKNOWN ) { Log::PrintF("^%u", Scancode);return; }

			if ( CodePoint & (1<<31) ) { return; }

			// The high bit of the scancode is set if the key is released.
			if ( Scancode & 0x80 ) { return; }

			if ( CodePoint & 0xFFFFFF80 ) { Log::PrintF("U+%x", CodePoint); return; }

			Log::PrintF("%s", &CodePoint); // Little endian hack!
		}

		void SetLEDs(uint8_t Toggle)
		{
			LEDs ^= Toggle;

			while ( (CPU::InPortB(0x64) & (1<<1)) != 0 ) { } //loop Until zero
			CPU::OutPortB(0x60, 0xED);
			while ( (CPU::InPortB(0x64) & (1<<1)) != 0 ) { } //loop Until zero
			CPU::OutPortB(0x60, LEDs);
		}
	}
}

