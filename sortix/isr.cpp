//
// isr.c -- High level interrupt service routines and interrupt request handlers.
//		  Part of this code is modified from Bran's kernel development tutorials.
//		  Rewritten for JamesM's kernel development tutorials.
//

#include "platform.h"
#include "globals.h"
#include "iprintable.h"
#include "iirqhandler.h"
#include "log.h"
#include "isr.h"
#include "panic.h"

size_t numknownexceptions = 19;
const char* exceptions[] = { "Divide by zero", "Debug", "Non maskable interrupt", "Breakpoint", 
                             "Into detected overflow", "Out of bounds", "Invalid opcode",
                             "No coprocessor", "Double fault", "Coprocessor segment overrun",
                             "Bad TSS", "Segment not present", "Stack fault",
                             "General protection fault", "Page fault", "Unknown interrupt",
                             "Coprocessor fault", "Alignment check", "Machine check" };

	Sortix::InterruptHandler interrupt_handlers[256];

	void register_interrupt_handler(uint8_t n, Sortix::InterruptHandler handler)
	{
		interrupt_handlers[n] = handler;
	}

	// This gets called from our ASM interrupt handler stub.
	extern "C" void isr_handler(Sortix::CPU::InterruptRegisters* Regs)
	{
#ifdef PLATFORM_X86
		if ( Regs->int_no < 32 )
		{
			const char* message = ( Regs->int_no < numknownexceptions ) ? exceptions[Regs->int_no] : "Unknown";
			Sortix::PanicF("Unhandled CPU Exception id %zu '%s' at eip=0x%zx (cr2=0x%p, err_code=0x%p)", Regs->int_no, message, Regs->eip, Regs->cr2, Regs->err_code);
		}

		if ( interrupt_handlers[Regs->int_no] != NULL )
		{
			interrupt_handlers[Regs->int_no](Regs);
		}
#else
		#warning "ISR handlers are not supported on this arch"
		while(true);
#endif
	}

	// This gets called from our ASM interrupt handler stub.
	extern "C" void irq_handler(Sortix::CPU::InterruptRegisters* Regs)
	{
#ifdef PLATFORM_X86
		if ( Regs->int_no != 32 && Regs->int_no != 33 )
		{
			//Sortix::Log::PrintF("IRQ eax=%u, int_no=%u, err_code=%u, eip=0x%x!\n", Regs->eax, Regs->int_no, Regs->err_code, Regs->eip);
		}

		if ( Regs->int_no < 32 || 48 < Regs->int_no ) { Sortix::PanicF("IRQ eax=%u, int_no=%u, err_code=%u, eip=%u!", Regs->eax, Regs->int_no, Regs->err_code, Regs->eip); }

		// TODO! IRQ 7 and 15 might be spurious and might need to be ignored.
		// See http://wiki.osdev.org/PIC for details (section Spurious IRQs).

		// Send an EOI (end of interrupt) signal to the PICs.		
		if (Regs->int_no >= 40) { Sortix::X86::OutPortB(0xA0, 0x20); } // Send reset signal to slave if this interrupt involved the slave.		
		Sortix::X86::OutPortB(0x20, 0x20); // Send reset signal to master.

		if ( interrupt_handlers[Regs->int_no] != NULL )
		{
			interrupt_handlers[Regs->int_no](Regs);
		}
#else
		#warning "IRQ handlers are not supported on this arch"
		while(true);
#endif
	}

