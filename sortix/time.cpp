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

	time.h
	Handles interrupts whenever some time has passed and provides useful
	time-related functions to the kernel.

******************************************************************************/

#include "platform.h"
#include "time.h"
#include "interrupt.h"
#include "scheduler.h"
#include "log.h"

#ifdef PLATFORM_SERIAL
#include <libmaxsi/keyboard.h>
#include "keyboard.h"
#include "vga.h"
#include "uart.h"
#endif

#if !defined(PLATFORM_X86_FAMILY)
#error No time subsystem is available for this CPU
#endif

namespace Sortix
{
	namespace Time
	{
		uintmax_t Ticks;
		uintmax_t Miliseconds;

		const uint32_t Frequency = 100; // 100 Hz

		void OnInt177(CPU::InterruptRegisters* Regs)
		{
#ifdef PLATFORM_X86
			Log::PrintF("ds=0x%x, edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x, ebx=0x%x, edx=0x%x, ecx=0x%x, eax=0x%x, int_no=0x%x, err_code=0x%x, eip=0x%x, cs=0x%x, eflags=0x%x, useresp=0x%x, ss=0x%x\n", Regs->ds, Regs->edi, Regs->esi, Regs->ebp, Regs->esp, Regs->ebx, Regs->edx, Regs->ecx, Regs->eax, Regs->int_no, Regs->err_code, Regs->eip, Regs->cs, Regs->eflags, Regs->useresp, Regs->ss);
#else
			#warning "INT 177 is not supported on this arch"
			while(true);
#endif
		}

		void RequestIQR0()
		{
			// The value we send to the PIT is the value to divide it's input clock
			// (1193180 Hz) by, to get our required frequency. Important to note is
			// that the divisor must be small enough to fit into 16-bits.
			const uint32_t Divisor = 1193180 / Frequency;

			// Send the command byte.
			CPU::OutPortB(0x43, 0x36);

			// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
			uint8_t L = (uint8_t) (Divisor & 0xFF);
			uint8_t H = (uint8_t) ((Divisor>>8) & 0xFF);

			// Send the frequency divisor.
			CPU::OutPortB(0x40, L);
			CPU::OutPortB(0x40, H);
		}

		bool didUglyIRQ0Hack;
		bool isEsc;
		bool isEscDepress;

		void Init()
		{
			// Initialize our variables.
			Ticks = 0;
			Miliseconds = 0;

			// First, register our timer callback.
			Interrupt::RegisterHandler(Interrupt::IRQ0, &OnIRQ0);
			Interrupt::RegisterHandler(177, &OnInt177);

			didUglyIRQ0Hack = false;

			RequestIQR0();

			isEsc = isEscDepress = false;
		}

		void OnIRQ0(CPU::InterruptRegisters* Regs)
		{
#ifdef PLATFORM_SERIAL
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
				if ( c == 3 ) { SigInt(); continue; }
				if ( c == 127 ) { c = '\b'; }
				if ( c & (1<<7) )
				{
					c &= ~(1<<7); c |= DEPRESSED;
				}
				Keyboard::QueueKeystroke(c);
			}

			// TODO: But this hack may be worse.
			UART::RenderVGA((VGA::Frame*) 0xB8000);
#endif

			Ticks++;

			// Let the scheduler switch to the next task.
			// TODO: Let the scheduler know how long has passed.
			Scheduler::Switch(Regs, 1000/Frequency);

			// TODO: There is a horrible bug that causes Sortix to only receive
			// one IRQ0 on my laptop, but it works in virtual machines. But 
			// re-requesting an addtional time seems to work. Hacky and ugly.
			if ( !didUglyIRQ0Hack ) { RequestIQR0(); didUglyIRQ0Hack = true; } 
		}

		// TODO: Implement all the other useful functions regarding tiem.
	}
}
