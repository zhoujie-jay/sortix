/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    time.cpp
    Handles interrupts whenever some time has passed and provides useful
    time-related functions to the kernel.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/interrupt.h>

#include "time.h"
#include "process.h"
#include "scheduler.h"
#include "sound.h"

#ifdef PLATFORM_SERIAL
#include "serialterminal.h"
#endif

#if !defined(PLATFORM_X86_FAMILY)
#error No time subsystem is available for this CPU
#endif

namespace Sortix
{
	namespace Time
	{
		uintmax_t ticks;
		uintmax_t microsecondssinceboot;

		const uint32_t Frequency = 100; // 100 Hz

		uintmax_t MicrosecondsSinceBoot()
		{
			return microsecondssinceboot;
		}

		extern "C" void RequestIRQ0()
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

		int SysUptime(uintmax_t* usecssinceboot)
		{
			// TODO: Validate that usecssinceboot is a valid user-space pointer.
			*usecssinceboot = microsecondssinceboot;
			return 0;
		}

		void Init()
		{
			// Initialize our variables.
			ticks = 0;
			microsecondssinceboot = 0;

			// First, register our timer callback.
			Interrupt::RegisterHandler(Interrupt::IRQ0, &OnIRQ0, NULL);

			Syscall::Register(SYSCALL_UPTIME, (void*) SysUptime);

			didUglyIRQ0Hack = false;

			RequestIRQ0();
		}

		void OnIRQ0(CPU::InterruptRegisters* Regs, void* /*user*/)
		{
#ifdef PLATFORM_SERIAL
			SerialTerminal::OnTick();
#endif

			ticks++;
			microsecondssinceboot += (1000*1000)/Frequency;

			// Let the scheduler switch to the next task.
			// TODO: Let the scheduler know how long has passed.
			Scheduler::Switch(Regs);

			// TODO: There is a horrible bug that causes Sortix to only receive
			// one IRQ0 on my laptop, but it works in virtual machines. But
			// re-requesting an addtional time seems to work. Hacky and ugly.
			if ( !didUglyIRQ0Hack ) { RequestIRQ0(); didUglyIRQ0Hack = true; }
		}

		// TODO: Implement all the other useful functions regarding time.
	}
}
