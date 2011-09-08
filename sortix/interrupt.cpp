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

	interrupt.cpp
	High level interrupt service routines and interrupt request handlers.

******************************************************************************/

#include "platform.h"
#include "log.h"
#include "interrupt.h"
#include "panic.h"

#include "process.h" // Hack for SIGSEGV
#include "sound.h" // Hack for SIGSEGV

namespace Sortix
{
	namespace Interrupt
	{
		size_t numknownexceptions = 19;
		const char* exceptions[] =
		{ "Divide by zero", "Debug", "Non maskable interrupt", "Breakpoint", 
		  "Into detected overflow", "Out of bounds", "Invalid opcode",
		  "No coprocessor", "Double fault", "Coprocessor segment overrun",
		  "Bad TSS", "Segment not present", "Stack fault",
		  "General protection fault", "Page fault", "Unknown interrupt",
		  "Coprocessor fault", "Alignment check", "Machine check"             };

		Handler interrupthandlers[256];

		void RegisterHandler(uint8_t n, Interrupt::Handler handler)
		{
			interrupthandlers[n] = handler;
		}

		// This gets called from our ASM interrupt handler stub.
		extern "C" void ISRHandler(Sortix::CPU::InterruptRegisters* regs)
		{
#ifdef PLATFORM_X86
			if ( regs->int_no < 32 )
			{
				const char* message = ( regs->int_no < numknownexceptions )
				                      ? exceptions[regs->int_no] : "Unknown";

				// Halt and catch fire if we are the kernel.
				if ( (regs->cs & (0x4-1)) == 0 || regs->int_no == 13 )
				{
					PanicF("Unhandled CPU Exception id %zu '%s' at eip=0x%zx "
					       "(cr2=0x%p, err_code=0x%p)", regs->int_no, message,
					       regs->eip, regs->cr2, regs->err_code);
				}

				Log::Print("The current program has crashed and was terminated:\n");
				Log::PrintF("%s exception at eip=0x%zx (cr2=0x%p, err_code=0x%p)\n",
				            message, regs->eip, regs->cr2, regs->err_code);

				Sound::Mute();
				const char* programname = "sh";
				regs->ebx = (uint32_t) programname;
				SysExecute(regs);
				return;
			}

			if ( interrupthandlers[regs->int_no] != NULL )
			{
				interrupthandlers[regs->int_no](regs);
			}
#else
			#warning "ISR handlers are not supported on this arch"
			while(true);
#endif
		}

		// This gets called from our ASM interrupt handler stub.
		extern "C" void IRQHandler(Sortix::CPU::InterruptRegisters* regs)
		{
#ifdef PLATFORM_X86
			// TODO! IRQ 7 and 15 might be spurious and might need to be ignored.
			// See http://wiki.osdev.org/PIC for details (section Spurious IRQs).
			if ( regs->int_no == 32 + 7 || regs->int_no == 32 + 15 ) { return; }

			if ( regs->int_no < 32 || 48 < regs->int_no )
			{
				PanicF("IRQ eax=%u, int_no=%u, err_code=%u, eip=%u!",
				       regs->eax, regs->int_no, regs->err_code, regs->eip);
			}

			// Send an EOI (end of interrupt) signal to the PICs.

			// Send reset signal to slave if this interrupt involved the slave.
			if (regs->int_no >= 40) { X86::OutPortB(0xA0, 0x20); } 

			// Send reset signal to master.
			X86::OutPortB(0x20, 0x20); 

			if ( interrupthandlers[regs->int_no] != NULL )
			{
				interrupthandlers[regs->int_no](regs);
			}
#else
			#warning "IRQ handlers are not supported on this arch"
			while(true);
#endif
		}
	}
}
