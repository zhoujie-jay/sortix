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
#include <libmaxsi/memory.h>
#include <libmaxsi/keyboard.h>
#include "log.h"
#include "panic.h"
#include "keyboard.h"
#include "interrupt.h"
#include "scheduler.h"
#include "syscall.h"

using namespace Maxsi::Keyboard;

namespace Sortix
{
	namespace Keyboard
	{
		namespace Layouts
		{
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

			nat Mask;
			nat LockMask;
			bool control;

			void Init()
			{
				Mask = 0;
				LockMask = 0;
				control = false;
			}

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
					if ( CodePoint == CTRL ) { control = false; }
				}
				else
				{
					if ( CodePoint == LSHFT ) { Mask |= Shift; }
					if ( CodePoint == ALTGR ) { Mask |= AltGr; }
					if ( CodePoint == SCRLCK ) { Mask |= ScrLck; }
					if ( CodePoint == CAPS ) { LockMask ^= Shift; SetLEDs(LEDCapsLck); }
					if ( CodePoint == CTRL ) { control = true; }
				}

				if ( control && ( CodePoint == 'c' || CodePoint == 'C' ) )
				{
					CodePoint = SIGINT;
				}

				return CodePoint;
			}
		}

		uint8_t LEDs;

		size_t keystrokeQueueOffset;
		size_t keystrokeQueueUsed;
		size_t keystrokeQueueLength;
		uint32_t* keystrokeQueue;

		uint32_t SysReceiveKeystroke()
		{
			if ( keystrokeQueueUsed == 0 ) { return 0; }

			uint32_t codepoint = keystrokeQueue[keystrokeQueueOffset++];
			keystrokeQueueOffset %= keystrokeQueueLength;
			keystrokeQueueUsed--;
			return codepoint;
		}

		void Init()
		{
			// Initialize variables.
			LEDs = 0;

			Layouts::Init();

			// Register our keystroke callback.
			Interrupt::RegisterHandler(Interrupt::IRQ1, OnIRQ1, NULL);

			// If any scancodes were already pending, our interrupt handler
			// will never be called. Let's just discard anything pending.
			CPU::InPortB(0x60);

			// Create a queue where pending keystrokes can be stored until
			// user-space applications access them.
			keystrokeQueueUsed = 0;
			keystrokeQueueOffset = 0;
			keystrokeQueueLength = 1024ULL;
			keystrokeQueue = new uint32_t[keystrokeQueueLength];
			if ( keystrokeQueue == NULL )
			{
				Panic("Could not allocate keystroke buffer");
			}

			Syscall::Register(SYSCALL_RECEIVE_KEYSTROKE, (void*) SysReceiveKeystroke);
		}

		bool QueueKeystroke(uint32_t keystroke)
		{
			if ( keystrokeQueueLength <= keystrokeQueueUsed ) { return false; }

			size_t position = keystrokeQueueOffset + keystrokeQueueUsed;
			position %= keystrokeQueueLength;
			keystrokeQueueUsed++;
			
			keystrokeQueue[position] = keystroke;

			return true;
		}

		void OnIRQ1(CPU::InterruptRegisters* Regs, void* user)
		{
			// Read the scancode from the data register.
			uint8_t Scancode = CPU::InPortB(0x60);

			//Log::PrintF("[%u]", Scancode);

			uint32_t CodePoint = Layouts::GetCodePoint(Scancode);

			if ( CodePoint == SIGINT )
			{
				Scheduler::SigIntHack();
				return;
			}

			bool KeyUp = (Scancode & 0x80);

			if ( KeyUp ) { CodePoint |= DEPRESSED; }

			QueueKeystroke(CodePoint);
			return;

			//if ( Reader != NULL ) { Reader->OnKeystroke(CodePoint, KeyUp); return; }

			// Use this to debug the exact scancodes you receive!
			//Log::PrintF("[%u/U+%x]", Scancode, CodePoint);

			if ( CodePoint == UNKNOWN ) { Log::PrintF("^%u", Scancode);return; }

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

