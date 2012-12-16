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

	kernel.cpp
	The main kernel initialization routine. Configures hardware and starts an
	initial process from the init ramdisk, allowing a full operating system.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/video.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/worker.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/mman.h>
#include <sortix/wait.h>
#include <errno.h>
#include <malloc.h>
#include "kernelinfo.h"
#include "x86-family/gdt.h"
#include "x86-family/float.h"
#include "time.h"
#include "keyboard.h"
#include "multiboot.h"
#include "thread.h"
#include "process.h"
#include "scheduler.h"
#include "signal.h"
#include "syscall.h"
#include "ata.h"
#include "com.h"
#include "uart.h"
#include "vgatextbuffer.h"
#include "terminal.h"
#include "serialterminal.h"
#include "textterminal.h"
#include "elf.h"
#include "initrd.h"
#include "vga.h"
#include "bga.h"
#include "sound.h"
#include "io.h"
#include "pipe.h"
#include "filesystem.h"
#include "mount.h"
#include "directory.h"
#include "interrupt.h"
#include "dispmsg.h"
#include "fs/devfs.h"

// Keep the stack size aligned with $CPU/base.s
const size_t STACK_SIZE = 64*1024;
extern "C" { size_t stack[STACK_SIZE / sizeof(size_t)] = {0}; }

namespace Sortix {

void DoMaxsiLogo()
{
	Log::Print("\e[37;41m\e[2J"); // Make the background color red.
	Log::Print("                                                       _                        \n");
	Log::Print("                                                      / \\                       \n");
	Log::Print("                  /\\    /\\                           /   \\                      \n");
	Log::Print("                 /  \\  /  \\                          |   |                      \n");
	Log::Print("                /    \\/    \\                         |   |                      \n");
	Log::Print("               |  O    O    \\_______________________ /   |                      \n");
	Log::Print("               |                                         |                      \n");
	Log::Print("               | \\_______/                               /                      \n");
	Log::Print("                \\                                       /                       \n");
	Log::Print("                  ------       ---------------      ---/                        \n");
	Log::Print("                       /       \\             /      \\                           \n");
	Log::Print("                      /         \\           /        \\                          \n");
	Log::Print("                     /           \\         /          \\                         \n");
	Log::Print("                    /_____________\\       /____________\\                        \n");
	Log::Print("                                                                                \n");
}

void DoWelcome()
{
	DoMaxsiLogo();
	Log::Print("                           BOOTING OPERATING SYSTEM...                          ");
}

// Forward declarations.
static void BootThread(void* user);
static void InitThread(void* user);
static void SystemIdleThread(void* user);

static size_t PrintToTextTerminal(void* user, const char* str, size_t len)
{
	return ((TextTerminal*) user)->Print(str, len);
}

static size_t TextTermWidth(void* user)
{
	return ((TextTerminal*) user)->Width();
}

static size_t TextTermHeight(void* user)
{
	return ((TextTerminal*) user)->Height();
}

extern "C" void KernelInit(unsigned long magic, multiboot_info_t* bootinfo)
{
	(void) magic;

	// Initialize system calls.
	Syscall::Init();

	// Detect and initialize any serial COM ports in the system.
	COM::EarlyInit();

	// Setup a text buffer handle for use by the text terminal.
	uint16_t* const VGAFB = (uint16_t*) 0xB8000;
	const size_t VGA_WIDTH = 80;
	const size_t VGA_HEIGHT = 25;
	static uint16_t vga_attr_buffer[VGA_WIDTH*VGA_HEIGHT];
	VGATextBuffer textbuf(VGAFB, vga_attr_buffer, VGA_WIDTH, VGA_HEIGHT);
	TextBufferHandle textbufhandle(NULL, false, &textbuf, false);

	// Setup a text terminal instance.
	TextTerminal textterm(&textbufhandle);

	// Register the text terminal as the kernel log and initialize it.
	Log::Init(PrintToTextTerminal, TextTermWidth, TextTermHeight, &textterm);

	// Display the boot welcome screen.
	DoWelcome();

#if defined(__x86_64__)
	// TODO: Remove this hack when qemu 1.4.x and 1.5.0 are obsolete.
	// Verify that we are not running under a buggy qemu where the instruction
	// movl (%eax), %esi is misinterpreted (amongst others). In this case it
	// will try to access the memory at [bx + si]. We'll make sure that eax
	// points to a variable on the stack that has another value than at bx + si,
	// and if the values compare equal using the buggy instruction, we panic.
	uint32_t intended_variable; // rax will point to here.
	uint32_t is_buggy_qemu;
	asm ("movq $0x1000, %%rbx\n" /* access 32-bit value at 0x1000 */
	     "movl (%%rbx), %%esi\n"
	     "subl $1, %%esi\n" /* change the 32-bit value */
	     "movl %%esi, (%%rax)\n" /* store the new value in intended_variable */
	     "movq $0x0, %%rsi\n" /* make rsi zero, so bx + si points to 0x1000 */
	     "movl (%%eax), %%esi\n" /* do the perhaps-buggy memory access */
	     "movl (%%rax), %%ebx\n" /* do a working memory access */
	     "movl %%ebx, %0\n" /* load the desired value into is_buggy_qemu */
	     "subl %%esi, %0\n" /* subtract the possibly incorrect value. */
	     : "=r"(is_buggy_qemu)
	     : "a"(&intended_variable)
	     : "rsi", "rbx");
	if ( is_buggy_qemu )
		Panic("You are running a buggy version of qemu.  The 1.4.x and 1.5.0 "
		      "releases are known to execute some instructions incorrectly on "
		      "x86_64 without KVM.  You have three options: 1) Enable KVM 2) "
		      "Use a 32-bit OS 3) Use another version of qemu.");
#endif

	if ( !bootinfo )
	{
		Panic("The bootinfo structure was NULL. Are your bootloader "
		      "multiboot compliant?");
	}

	addr_t initrd = 0;
	size_t initrdsize = 0;

	uint32_t* modules = (uint32_t*) (addr_t) bootinfo->mods_addr;
	for ( uint32_t i = 0; i < bootinfo->mods_count; i++ )
	{
		initrdsize = modules[2*i+1] - modules[2*i+0];
		initrd = (addr_t) modules[2*i+0];
		break;
	}

	if ( !initrd ) { PanicF("No init ramdisk provided"); }

	Memory::RegisterInitRDSize(initrdsize);

	// Initialize paging and virtual memory.
	Memory::Init(bootinfo);

	// Initialize the GDT and TSS structures.
	GDT::Init();

	// Initialize the interrupt handler table and enable interrupts.
	Interrupt::Init();

	// Initialize the kernel heap.
	_init_heap();

	// Initialize the interrupt worker.
	Interrupt::InitWorker();

	// Initialize the list of kernel devices.
	DeviceFS::Init();

	// Initialize the COM ports.
	COM::Init();

	// Initialize the keyboard.
	Keyboard::Init();

	// Initialize the terminal.
	Terminal::Init();

	// Initialize the VGA driver.
	VGA::Init();

	// Initialize the sound driver.
	Sound::Init();

	// Initialize the process system.
	Process::Init();

	// Initialize the thread system.
	Thread::Init();

	// Initialize the IO system.
	IO::Init();

	// Initialize the pipe system.
	Pipe::Init();

	// Initialize the filesystem system.
	FileSystem::Init();

	// Initialize the directory system.
	Directory::Init();

	// Initialize the mount system.
	Mount::Init();

	// Initialize the scheduler.
	Scheduler::Init();

	// Initialize Unix Signals.
	Signal::Init();

	// Initialize the worker thread data structures.
	Worker::Init();

	// Initialize the kernel information query syscall.
	Info::Init();

	// Set up the initial ram disk.
	InitRD::Init(initrd, initrdsize);

	// Initialize the Video Driver framework.
	Video::Init(&textbufhandle);

	// Search for PCI devices and load their drivers.
	PCI::Init();

	// Initialize ATA devices.
	ATA::Init();

	// Initialize the BGA driver.
	BGA::Init();

	// Initialize the Display Message framework.
	DisplayMessage::Init();

	// Now that the base system has been loaded, it's time to go threaded. First
	// we create an object that represents this thread.
	Process* system = new Process;
	if ( !system ) { Panic("Could not allocate the system process"); }
	addr_t systemaddrspace = Memory::GetAddressSpace();
	system->addrspace = systemaddrspace;

	// We construct this thread manually for bootstrap reasons. We wish to
	// create a kernel thread that is the current thread and isn't put into the
	// scheduler's set of runnable threads, but rather run whenever there is
	// _nothing_ else to run on this CPU.
	Thread* idlethread = new Thread;
	idlethread->process = system;
	idlethread->kernelstackpos = (addr_t) stack;
	idlethread->kernelstacksize = STACK_SIZE;
	idlethread->kernelstackmalloced = false;
	idlethread->fpuinitialized = true;
	system->firstthread = idlethread;
	Scheduler::SetIdleThread(idlethread);

	// Let's create a regular kernel thread that can decide what happens next.
	// Note that we don't do the work here: should it block, then there is
	// nothing to run. Therefore we must become the system idle thread.
	RunKernelThread(BootThread, NULL);

	// Set up such that floating point registers are lazily switched.
	Float::Init();

	// The time driver will run the scheduler on the next timer interrupt.
	Time::Init();

	// Become the system idle thread.
	SystemIdleThread(NULL);
}

static void SystemIdleThread(void* /*user*/)
{
	// Alright, we are now the system idle thread. If there is nothing to do,
	// then we are run. Note that we must never do any real work here.
	while(true);
}

static void BootThread(void* /*user*/)
{
	// Hello, threaded world! You can now regard the kernel as a multi-threaded
	// process with super-root access to the system. Before we boot the full
	// system we need to start some worker threads.

	// Let's create the interrupt worker thread that executes additional work
	// requested by interrupt handlers, where such work isn't safe.
	Thread* interruptworker = RunKernelThread(Interrupt::WorkerThread, NULL);
	if ( !interruptworker )
		Panic("Could not create interrupt worker");

	// Create a general purpose worker thread.
	Thread* workerthread = RunKernelThread(Worker::Thread, NULL);
	if ( !workerthread )
		Panic("Unable to create general purpose worker thread");

	// Finally, let's transfer control to a new kernel process that will
	// eventually run user-space code known as the operating system.
	addr_t initaddrspace = Memory::Fork();
	if ( !initaddrspace ) { Panic("Could not create init's address space"); }

	Process* init = new Process;
	if ( !init ) { Panic("Could not allocate init process"); }

	CurrentProcess()->AddChildProcess(init);

	init->addrspace = initaddrspace;
	Scheduler::SetInitProcess(init);

	Thread* initthread = RunKernelThread(init, InitThread, NULL);
	if ( !initthread ) { Panic("Coul not create init thread"); }

	// Wait until init init is done and then shut down the computer.
	int status;
	pid_t pid = CurrentProcess()->Wait(init->pid, &status, 0);
	if ( pid != init->pid )
		PanicF("Waiting for init to exit returned %i (errno=%i)", pid, errno);

	status = WEXITSTATUS(status);

	switch ( status )
	{
	case 0: CPU::ShutDown();
	case 1: CPU::Reboot();
	default:
		PanicF("Init returned with unexpected return code %i", status);
	}
}

static void InitThread(void* /*user*/)
{
	// We are the init process's first thread. Let's load the init program from
	// the init ramdisk and transfer execution to it. We will then become a
	// regular user-space program with root permissions.

	Thread* thread = CurrentThread();
	Process* process = CurrentProcess();

	uint32_t inode = InitRD::Traverse(InitRD::Root(), "init");
	if ( !inode ) { Panic("InitRD did not contain an 'init' program."); }

	size_t programsize;
	uint8_t* program = InitRD::Open(inode, &programsize);
	if ( !program ) { Panic("InitRD did not contain an 'init' program."); }

	const size_t DEFAULT_STACK_SIZE = 64UL * 1024UL;

	size_t stacksize = 0;
	if ( !stacksize ) { stacksize = DEFAULT_STACK_SIZE; }

	addr_t stackpos = process->AllocVirtualAddr(stacksize);
	if ( !stackpos ) { Panic("Could not allocate init stack space"); }

	int prot = PROT_FORK | PROT_READ | PROT_WRITE | PROT_KREAD | PROT_KWRITE;
	if ( !Memory::MapRange(stackpos, stacksize, prot) )
	{
		Panic("Could not allocate init stack memory");
	}

	thread->stackpos = stackpos;
	thread->stacksize = stacksize;

	int argc = 1;
	const char* argv[] = { "init", NULL };
#if defined(PLATFORM_X86)
	const char* cputype = "cputype=i486-sortix";
#elif defined(PLATFORM_X64)
	const char* cputype = "cputype=x86_64-sortix";
#else
	#warning No cputype environmental variable provided here.
	const char* cputype = "cputype=unknown-sortix";
#endif
	int envc = 1;
	const char* envp[] = { cputype, NULL };
	CPU::InterruptRegisters regs;

	if ( process->Execute("init", program, programsize, argc, argv, envc, envp,
                          &regs) )
	{
		Panic("Unable to execute init program");
	}

	// Now become the init process and the operation system shall run.
	CPU::LoadRegisters(&regs);
}

} // namespace Sortix
