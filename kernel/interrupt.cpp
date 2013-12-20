/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/calltrace.h>
#include <sortix/kernel/cpu.h>
#include <sortix/kernel/debugger.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>

#include "x86-family/idt.h"

namespace Sortix {

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

extern "C" { unsigned long asm_is_cpu_interrupted = 0; }
const bool DEBUG_EXCEPTION = true;
const bool DEBUG_IRQ = false;
const bool DEBUG_ISR = false;
const bool CALLTRACE_KERNEL = false;
const bool CALLTRACE_USER = false;
const bool RUN_DEBUGGER_ON_CRASH = false;

const size_t NUM_KNOWN_EXCEPTIONS = 20;
const char* exceptions[] =
{
	"Divide by zero",              /* 0, 0x0 */
	"Debug",                       /* 1, 0x1 */
	"Non maskable interrupt",      /* 2, 0x2 */
	"Breakpoint",                  /* 3, 0x3 */
	"Into detected overflow",      /* 4, 0x4 */
	"Out of bounds",               /* 5, 0x5 */
	"Invalid opcode",              /* 6, 0x6 */
	"No coprocessor",              /* 7, 0x7 */
	"Double fault",                /* 8, 0x8 */
	"Coprocessor segment overrun", /* 9, 0x9 */
	"Bad TSS",                     /* 10, 0xA */
	"Segment not present",         /* 11, 0xB */
	"Stack fault",                 /* 12, 0xC */
	"General protection fault",    /* 13, 0xD */
	"Page fault",                  /* 14, 0xE */
	"Unknown interrupt",           /* 15, 0xF */
	"Coprocessor fault",           /* 16, 0x10 */
	"Alignment check",             /* 17, 0x11 */
	"Machine check",               /* 18, 0x12 */
	"SIMD Floating-Point",         /* 19, 0x13 */
};

const unsigned int NUM_INTERRUPTS = 256UL;
static Handler interrupt_handlers[NUM_INTERRUPTS];
static void* interrupt_handler_params[NUM_INTERRUPTS];

extern "C" void ReprogramPIC()
{
	uint8_t master_mask = 0;
	uint8_t slave_mask = 0;
	CPU::OutPortB(PIC_MASTER + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_SLAVE + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, IRQ0);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, IRQ8);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, 0x04); // Slave PIC at IRQ2
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, 0x02); // Cascade Identity
	CPU::OutPortB(PIC_MASTER + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, master_mask);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, slave_mask);
}

extern "C" void DeprogramPIC()
{
	uint8_t master_mask = 0;
	uint8_t slave_mask = 0;
	CPU::OutPortB(PIC_MASTER + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_SLAVE + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, 0x08);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, 0x70);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, 0x04); // Slave PIC at IRQ2
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, 0x02); // Cascade Identity
	CPU::OutPortB(PIC_MASTER + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, PIC_MODE_8086);
	CPU::OutPortB(PIC_MASTER + PIC_DATA, master_mask);
	CPU::OutPortB(PIC_SLAVE + PIC_DATA, slave_mask);
}

void Init()
{
	IDT::Init();

	for ( unsigned int i = 0; i < NUM_INTERRUPTS; i++ )
	{
		interrupt_handlers[i] = NULL;
		interrupt_handler_params[i] = NULL;
		RegisterRawHandler(i, interrupt_handler_null, false);
	}

	// Remap the IRQ table on the PICs.
	ReprogramPIC();

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

	Interrupt::Enable();
}

void RegisterHandler(unsigned int index, Interrupt::Handler handler, void* user)
{
	interrupt_handlers[index] = handler;
	interrupt_handler_params[index] = user;
}

// TODO: This function contains magic IDT-related values!
void RegisterRawHandler(unsigned int index, RawHandler handler, bool userspace)
{
	addr_t handler_entry = (addr_t) handler;
	uint16_t sel = KCS;
	uint8_t flags = 0x8E;
	if ( userspace )
		flags |= 0x60;
	IDT::SetEntry(index, handler_entry, sel, flags);
}

void CrashHandler(CPU::InterruptRegisters* regs)
{
	CurrentThread()->SaveRegisters(regs);

	const char* message = regs->int_no < NUM_KNOWN_EXCEPTIONS
	                      ? exceptions[regs->int_no] : "Unknown";

	if ( DEBUG_EXCEPTION )
	{
		regs->LogRegisters();
		Log::Print("\n");
	}

#if defined(__x86_64__)
	addr_t ip = regs->rip;
#elif defined(__i386__)
	addr_t ip = regs->eip;
#endif

	// Halt and catch fire if we are the kernel.
	unsigned code_mode = regs->cs & 0x3;
	bool is_in_kernel = !code_mode;
	bool is_in_user = !is_in_kernel;

	if ( (is_in_kernel && CALLTRACE_KERNEL) || (is_in_user && CALLTRACE_USER) )
	#if defined(__x86_64__)
		Calltrace::Perform(regs->rbp);
	#elif defined(__i386__)
		Calltrace::Perform(regs->ebp);
	#else
		#error Please provide a calltrace implementation for your CPU.
	#endif

	if ( RUN_DEBUGGER_ON_CRASH )
		Debugger::Run();

	if ( is_in_kernel )
	{
		PanicF("Unhandled CPU Exception id %zu '%s' at ip=0x%zx "
		       "(cr2=0x%p, err_code=0x%p)", regs->int_no, message,
		       ip, regs->cr2, regs->err_code);
	}

	Interrupt::Enable();

	Log::PrintF("The current program (pid %ji %s) has crashed and was terminated:\n",
	            (intmax_t) CurrentProcess()->pid, CurrentProcess()->program_image_path);
	Log::PrintF("%s exception at ip=0x%zx (cr2=0x%p, err_code=0x%p)\n",
	            message, ip, regs->cr2, regs->err_code);

	// Exit the process with the right error code.
	// TODO: Sent a SIGINT, SIGBUS, or whatever instead.
	CurrentProcess()->Exit(139);

	Interrupt::Disable();
	Signal::Dispatch(regs);
}

void ISRHandler(CPU::InterruptRegisters* regs)
{
	unsigned int int_no = regs->int_no;

	if ( DEBUG_ISR )
	{
		Log::PrintF("ISR%u ", int_no);
		regs->LogRegisters();
		Log::Print("\n");
	}

	// Run the desired interrupt handler.
	if ( int_no < 32 && int_no != 7 )
		CrashHandler(regs);
	else if ( interrupt_handlers[regs->int_no] != NULL )
		interrupt_handlers[int_no](regs, interrupt_handler_params[int_no]);
}

void IRQHandler(CPU::InterruptRegisters* regs)
{
	// TODO: IRQ 7 and 15 might be spurious and might need to be ignored.
	// See http://wiki.osdev.org/PIC for details (section Spurious IRQs).
	if ( regs->int_no == 32 + 7 || regs->int_no == 32 + 15 )
		return;

	if ( DEBUG_IRQ )
	{
		Log::PrintF("IRQ%u ", regs->int_no-32);
		regs->LogRegisters();
		Log::Print("\n");
	}

	unsigned int int_no = regs->int_no;

	// Send an EOI (end of interrupt) signal to the PICs.
	if ( IRQ8 <= int_no )
		CPU::OutPortB(PIC_SLAVE, PIC_CMD_ENDINTR);
	CPU::OutPortB(PIC_MASTER, PIC_CMD_ENDINTR);

	if ( interrupt_handlers[int_no] )
		interrupt_handlers[int_no](regs, interrupt_handler_params[int_no]);
}

extern "C" void interrupt_handler(CPU::InterruptRegisters* regs)
{
	if ( 32 <= regs->int_no && regs->int_no < 48 )
		IRQHandler(regs);
	else
		ISRHandler(regs);
}

// TODO: This implementation is a bit hacky and can be optimized.

uint8_t* queue;
uint8_t* storage;
volatile size_t queueoffset;
volatile size_t queueused;
size_t queuesize;

struct Package
{
	size_t size;
	size_t payloadoffset;
	size_t payloadsize;
	WorkHandler handler; // TODO: May not be correctly aligned on some systems.
	uint8_t payload[0];
};

void InitWorker()
{
	const size_t QUEUE_SIZE = 4UL*1024UL;
	STATIC_ASSERT(QUEUE_SIZE % sizeof(Package) == 0);
	queue = new uint8_t[QUEUE_SIZE];
	if ( !queue )
		Panic("Can't allocate interrupt worker queue");
	storage = new uint8_t[QUEUE_SIZE];
	if ( !storage )
		Panic("Can't allocate interrupt worker storage");
	queuesize = QUEUE_SIZE;
	queueoffset = 0;
	queueused = 0;
}

static void WriteToQueue(const void* src, size_t size)
{
	const uint8_t* buf = (const uint8_t*) src;
	size_t writeat = (queueoffset + queueused) % queuesize;
	size_t available = queuesize - writeat;
	size_t count = available < size ? available : size;
	memcpy(queue + writeat, buf, count);
	queueused += count;
	if ( count < size )
		WriteToQueue(buf + count, size - count);
}

static void ReadFromQueue(void* dest, size_t size)
{
	uint8_t* buf = (uint8_t*) dest;
	size_t available = queuesize - queueoffset;
	size_t count = available < size ? available : size;
	memcpy(buf, queue + queueoffset, count);
	queueused -= count;
	queueoffset = (queueoffset + count) % queuesize;
	if ( count < size )
		ReadFromQueue(buf + count, size - count);
}

static Package* PopPackage(uint8_t** payloadp, Package* /*prev*/)
{
	Package* package = NULL;
	uint8_t* payload = NULL;
	Interrupt::Disable();

	if ( !queueused )
		goto out;

	package = (Package*) storage;
	ReadFromQueue(package, sizeof(*package));
	payload = storage + sizeof(*package);
	ReadFromQueue(payload, package->payloadsize);
	*payloadp = payload;

out:
	Interrupt::Enable();
	return package;
}

void WorkerThread(void* /*user*/)
{
	assert(Interrupt::IsEnabled());
	uint8_t* payload = NULL;
	Package* package = NULL;
	while ( true )
	{
		package = PopPackage(&payload, package);
		if ( !package ) { Scheduler::Yield(); continue; }
		size_t payloadsize = package->payloadsize;
		package->handler(payload, payloadsize);
	}
}

bool ScheduleWork(WorkHandler handler, void* payload, size_t payloadsize)
{
	assert(!Interrupt::IsEnabled());

	Package package;
	package.size = sizeof(package) + payloadsize;
	package.payloadoffset = 0; // Currently unused
	package.payloadsize = payloadsize;
	package.handler = handler;

	size_t queuefreespace = queuesize - queueused;
	if ( queuefreespace < package.size )
		return false;

	WriteToQueue(&package, sizeof(package));
	WriteToQueue(payload, payloadsize);
	return true;
}

} // namespace Interrupt
} // namespace Sortix
