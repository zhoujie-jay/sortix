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

    sortix/kernel/interrupt.h
    High level interrupt service routines and interrupt request handlers.

*******************************************************************************/

#ifndef SORTIX_INTERRUPT_H
#define SORTIX_INTERRUPT_H

#include "cpu.h"

namespace Sortix {
namespace Interrupt {

const unsigned IRQ0 = 32;
const unsigned IRQ1 = 33;
const unsigned IRQ2 = 34;
const unsigned IRQ3 = 35;
const unsigned IRQ4 = 36;
const unsigned IRQ5 = 37;
const unsigned IRQ6 = 38;
const unsigned IRQ7 = 39;
const unsigned IRQ8 = 40;
const unsigned IRQ9 = 41;
const unsigned IRQ10 = 42;
const unsigned IRQ11 = 43;
const unsigned IRQ12 = 44;
const unsigned IRQ13 = 45;
const unsigned IRQ14 = 46;
const unsigned IRQ15 = 47;

extern "C" unsigned long asm_interrupts_are_enabled();
extern "C" unsigned long asm_is_cpu_interrupted;

inline bool IsEnabled() { return asm_interrupts_are_enabled(); }
inline void Enable() { asm volatile("sti"); }
inline void Disable() { asm volatile("cli"); }
inline bool IsCPUInterrupted() { return asm_is_cpu_interrupted != 0; }
inline bool SetEnabled(bool isenabled)
{
	bool wasenabled = IsEnabled();
	if ( isenabled ) { Enable(); } else { Disable(); }
	return wasenabled;
}

typedef void (*Handler)(CPU::InterruptRegisters* regs, void* user);
void RegisterHandler(unsigned index, Handler handler, void* user);

typedef void (*RawHandler)(void);
void RegisterRawHandler(unsigned index, RawHandler handler, bool userspace);

void Init();
void InitWorker();
void WorkerThread(void* user);

typedef void(*WorkHandler)(void* payload, size_t payloadsize);
bool ScheduleWork(WorkHandler handler, void* payload, size_t payloadsize);

} // namespace Interrupt
} // namespace Sortix

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void isr128();
extern "C" void isr130();
extern "C" void isr131();
extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();
extern "C" void interrupt_handler_null();

#endif
