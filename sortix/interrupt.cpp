/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	interrupt.cpp
	High level interrupt service routines and interrupt request handlers.

*******************************************************************************/

#include "platform.h"
#include "x86-family/idt.h"
#include "interrupt.h"

#include "process.h" // Hack for SIGSEGV
#include "sound.h" // Hack for SIGSEGV
#include "thread.h" // HACK FOR SIGSEGV
#include "syscall.h" // HACK FOR SIGSEGV
#include "scheduler.h" // HACK FOR SIGSEGV

namespace Sortix {
void SysExit(int status); // HACK

namespace Interrupt {

const uint16_t PIC_MASTER = 0x20;
const uint16_t PIC_SLAVE = 0xA0;
const uint16_t PIC_COMMAND = 0x00;
const uint16_t PIC_DATA = 0x01;
const uint8_t PIC_CMD_ENDINTR = 0x20;
const uint8_t PIC_ICW1_ICW4 = 0x01; // ICW4 (not) needed
const uint8_t PIC_ICW1_SINGLE = 0x02; // Single (cascade) mode
const uint8_t PIC_ICW1_INTERVAL4 = 0x04; // Call address interval 4 (8)
const uint8_t PIC_ICW1_LEVEL = 0x08; // Level triggered (edge) mode
const uint8_t PIC_CMD_INIT = 0x10;
const uint8_t PIC_MODE_8086 = 0x01; // 8086/88 (MCS-80/85) mode
const uint8_t PIC_MODE_AUTO = 0x02; // Auto (normal) EOI
const uint8_t PIC_MODE_BUF_SLAVE = 0x08; // Buffered mode/slave
const uint8_t PIC_MODE_BUF_MASTER = 0x0C; // Buffered mode/master
const uint8_t PIC_MODE_SFNM = 0x10; // Special fully nested (not)

const bool DEBUG_EXCEPTION = false;
const bool DEBUG_IRQ = false;
bool initialized;

const size_t NUM_KNOWN_EXCEPTIONS = 20;
const char* exceptions[] =
{
	"Divide by zero",
	"Debug",
	"Non maskable interrupt",
	"Breakpoint",
	"Into detected overflow",
	"Out of bounds",
	"Invalid opcode",
	"No coprocessor",
	"Double fault",
	"Coprocessor segment overrun",
	"Bad TSS",
	"Segment not present",
	"Stack fault",
	"General protection fault",
	"Page fault",
	"Unknown interrupt",
	"Coprocessor fault",
	"Alignment check",
	"Machine check",
	"SIMD Floating-Point",
};

const unsigned NUM_INTERRUPTS = 256UL;

Handler interrupthandlers[NUM_INTERRUPTS];
void* interrupthandlerptr[NUM_INTERRUPTS];

void Init()
{
	initialized = false;
	IDT::Init();

	for ( unsigned i = 0; i < NUM_INTERRUPTS; i++ )
	{
		interrupthandlers[i] = NULL;
		interrupthandlerptr[i] = NULL;
		RegisterRawHandler(i, interrupt_handler_null, false);
	}

	// Remap the IRQ table on the PICs.
	uint8_t mastermask = 0;
	uint8_t slavemask = 0;
	CPU::OutPortB(PIC_MASTER + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_SLAVE + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, IRQ0);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, IRQ8);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, 0x04); // Slave PIC at IRQ2
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, 0x02); // Cascade Identity
	CPU::OutPortB(PIC_MASTER + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, mastermask);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, slavemask);

	RegisterRawHandler(0, isr0, false);
	RegisterRawHandler(1, isr1, false);
	RegisterRawHandler(2, isr2, false);
	RegisterRawHandler(3, isr3, false);
	RegisterRawHandler(4, isr4, false);
	RegisterRawHandler(5, isr5, false);
	RegisterRawHandler(6, isr6, false);
	RegisterRawHandler(7, isr7, false);
	RegisterRawHandler(8, isr8, false);
	RegisterRawHandler(9, isr9, false);
	RegisterRawHandler(10, isr10, false);
	RegisterRawHandler(11, isr11, false);
	RegisterRawHandler(12, isr12, false);
	RegisterRawHandler(13, isr13, false);
	RegisterRawHandler(14, isr14, false);
	RegisterRawHandler(15, isr15, false);
	RegisterRawHandler(16, isr16, false);
	RegisterRawHandler(17, isr17, false);
	RegisterRawHandler(18, isr18, false);
	RegisterRawHandler(19, isr19, false);
	RegisterRawHandler(20, isr20, false);
	RegisterRawHandler(21, isr21, false);
	RegisterRawHandler(22, isr22, false);
	RegisterRawHandler(23, isr23, false);
	RegisterRawHandler(24, isr24, false);
	RegisterRawHandler(25, isr25, false);
	RegisterRawHandler(26, isr26, false);
	RegisterRawHandler(27, isr27, false);
	RegisterRawHandler(28, isr28, false);
	RegisterRawHandler(29, isr29, false);
	RegisterRawHandler(30, isr30, false);
	RegisterRawHandler(31, isr31, false);
	RegisterRawHandler(32, irq0, false);
	RegisterRawHandler(33, irq1, false);
	RegisterRawHandler(34, irq2, false);
	RegisterRawHandler(35, irq3, false);
	RegisterRawHandler(36, irq4, false);
	RegisterRawHandler(37, irq5, false);
	RegisterRawHandler(38, irq6, false);
	RegisterRawHandler(39, irq7, false);
	RegisterRawHandler(40, irq8, false);
	RegisterRawHandler(41, irq9, false);
	RegisterRawHandler(42, irq10, false);
	RegisterRawHandler(43, irq11, false);
	RegisterRawHandler(44, irq12, false);
	RegisterRawHandler(45, irq13, false);
	RegisterRawHandler(46, irq14, false);
	RegisterRawHandler(47, irq15, false);

	// TODO: Let the syscall.cpp code register this.
	RegisterRawHandler(128, syscall_handler, true);

	IDT::Flush();
	initialized = true;

	Interrupt::Enable();
}

void RegisterHandler(unsigned n, Interrupt::Handler handler, void* user)
{
	interrupthandlers[n] = handler;
	interrupthandlerptr[n] = user;
}

// TODO: This function contains magic IDT-related values!
void RegisterRawHandler(unsigned index, RawHandler handler, bool userspace)
{
	addr_t handlerentry = (addr_t) handler;
	uint16_t sel = 0x08;
	uint8_t flags = 0x8E;
	if ( userspace ) { flags |= 0x60; }
	IDT::SetGate(index, handlerentry, sel, flags);
	if ( initialized ) { IDT::Flush(); }
}

void CrashHandler(CPU::InterruptRegisters* regs)
{
	const char* message = ( regs->int_no < NUM_KNOWN_EXCEPTIONS )
	                      ? exceptions[regs->int_no] : "Unknown";

	if ( DEBUG_EXCEPTION ) { regs->LogRegisters(); Log::Print("\n"); }

#ifdef PLATFORM_X64
	addr_t ip = regs->rip;
#else
	addr_t ip = regs->eip;
#endif

	// Halt and catch fire if we are the kernel.
	unsigned codemode = regs->cs & 0x3U;
	if ( codemode == 0 )
	{
		PanicF("Unhandled CPU Exception id %zu '%s' at ip=0x%zx "
		       "(cr2=0x%p, err_code=0x%p)", regs->int_no, message,
		       ip, regs->cr2, regs->err_code);
	}

	Log::Print("The current program has crashed and was terminated:\n");
	Log::PrintF("%s exception at ip=0x%zx (cr2=0x%p, err_code=0x%p)\n",
	            message, ip, regs->cr2, regs->err_code);

	Sound::Mute();

	CurrentProcess()->Exit(139);
	Scheduler::ProcessTerminated(regs);
}

void ISRHandler(Sortix::CPU::InterruptRegisters* regs)
{
	if ( regs->int_no < 32 )
	{
		CrashHandler(regs);
		return;
	}

	if ( interrupthandlers[regs->int_no] != NULL )
	{
		void* user = interrupthandlerptr[regs->int_no];
		interrupthandlers[regs->int_no](regs, user);
	}
}

void IRQHandler(Sortix::CPU::InterruptRegisters* regs)
{
	// TODO: IRQ 7 and 15 might be spurious and might need to be ignored.
	// See http://wiki.osdev.org/PIC for details (section Spurious IRQs).
	if ( regs->int_no == 32 + 7 || regs->int_no == 32 + 15 ) { return; }

	if ( DEBUG_IRQ )
	{
		Log::PrintF("IRQ%u ", regs->int_no-32);
		regs->LogRegisters();
		Log::Print("\n");
	}

	unsigned int_no = regs->int_no;

	// Send an EOI (end of interrupt) signal to the PICs.
	if ( IRQ8 <= int_no ) { CPU::OutPortB(PIC_SLAVE, PIC_CMD_ENDINTR); } 
	CPU::OutPortB(PIC_MASTER, PIC_CMD_ENDINTR);

	if ( interrupthandlers[int_no] )
	{
		void* user = interrupthandlerptr[int_no];
		interrupthandlers[int_no](regs, user);
	}
}

extern "C" void interrupt_handler(Sortix::CPU::InterruptRegisters* regs)
{
	size_t int_no = regs->int_no;
	if ( 32 <= int_no && int_no < 48 ) { IRQHandler(regs); }
	else { ISRHandler(regs); }
}

} // namespace Interrupt
} // namespace Sortix

