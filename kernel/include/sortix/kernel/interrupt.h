/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    sortix/kernel/interrupt.h
    High level interrupt services.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_INTERRUPT_H
#define INCLUDE_SORTIX_KERNEL_INTERRUPT_H

#include <stddef.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/registers.h>

namespace Sortix {
namespace Interrupt {

#if defined(__i386__) || defined(__x86_64__)
const unsigned int IRQ0 = 32;
const unsigned int IRQ1 = 33;
const unsigned int IRQ2 = 34;
const unsigned int IRQ3 = 35;
const unsigned int IRQ4 = 36;
const unsigned int IRQ5 = 37;
const unsigned int IRQ6 = 38;
const unsigned int IRQ7 = 39;
const unsigned int IRQ8 = 40;
const unsigned int IRQ9 = 41;
const unsigned int IRQ10 = 42;
const unsigned int IRQ11 = 43;
const unsigned int IRQ12 = 44;
const unsigned int IRQ13 = 45;
const unsigned int IRQ14 = 46;
const unsigned int IRQ15 = 47;
#endif

extern "C" unsigned long asm_is_cpu_interrupted;

inline bool IsEnabled()
{
#if defined(__i386__) || defined(__x86_64__)
	unsigned long is_enabled;
	asm("pushf\t\n"
	    "pop %0\t\n"
	    "and $0x000200, %0" : "=r"(is_enabled));
	return is_enabled != 0;
#else
#warning "You need to implement checking if interrupts are on"
#endif
}

inline void Enable()
{
#if defined(__i386__) || defined(__x86_64__)
	asm volatile("sti");
#else
#warning "You need to implement enabling interrupts"
#endif
}

inline void Disable()
{
#if defined(__i386__) || defined(__x86_64__)
	asm volatile("cli");
#else
#warning "You need to implement disabling interrupts"
#endif
}

inline bool IsCPUInterrupted()
{
	return asm_is_cpu_interrupted != 0;
}

inline bool SetEnabled(bool is_enabled)
{
	bool wasenabled = IsEnabled();
	if ( is_enabled )
		Enable();
	else
		Disable();
	return wasenabled;
}


typedef void (*Handler)(struct interrupt_context* intctx, void* user);
void RegisterHandler(unsigned int index, Handler handler, void* user);

void Init();
void InitWorker();
void WorkerThread(void* user);

bool ScheduleWork(void (*handler)(void*, void*, size_t),
                  void* handler_context,
                  void* payload,
                  size_t payload_size);

} // namespace Interrupt
} // namespace Sortix

#endif
