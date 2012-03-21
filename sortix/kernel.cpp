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

	kernel.cpp
	The main kernel initialization routine. Configures hardware and starts an
	initial process from the init ramdisk, allowing a full operating system.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include <libmaxsi/format.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#include "kernelinfo.h"
#include "x86-family/gdt.h"
#include "time.h"
#include "keyboard.h"
#include "multiboot.h"
#include <sortix/kernel/memorymanagement.h>
#include "thread.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"
#include "pci.h"
#include "com.h"
#include "uart.h"
#include "terminal.h"
#include "serialterminal.h"
#include "vgaterminal.h"
#include "elf.h"
#include "initrd.h"
#include "vga.h"
#include "sound.h"
#include "io.h"
#include "pipe.h"
#include "filesystem.h"
#include "mount.h"
#include "directory.h"
#include "interrupt.h"
#include "fs/devfs.h"

using namespace Maxsi;

// Keep the stack size aligned with $CPU/base.s
extern "C" { size_t stack[64*1024 / sizeof(size_t)] = {0}; }

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

extern "C" void KernelInit(unsigned long magic, multiboot_info_t* bootinfo)
{
	// Initialize system calls.
	Syscall::Init();

	// Detect and initialize any serial COM ports in the system.
	COM::EarlyInit();

	// Initialize the default terminal.
	VGATerminal::Init();
	Maxsi::Format::Callback logcallback = VGATerminal::Print;
	void* logpointer = NULL;

	// Initialize the kernel log.
	Log::Init(logcallback, logpointer);

	// Display the boot welcome screen.
	DoWelcome();

	if ( !bootinfo )
	{
		Panic("The bootinfo structure was NULL. Are your bootloader "
		      "multiboot compliant?");
	}

	addr_t initrd = NULL;
	size_t initrdsize = 0;

	uint32_t* modules = (uint32_t*) bootinfo->mods_addr;
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
	Maxsi::Memory::Init();

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

	// Initialize the kernel information query syscall.
	Info::Init();

	// Set up the initial ram disk.
	InitRD::Init(initrd, initrdsize);

	// Search for PCI devices and load their drivers.
	PCI::Init();

	// Alright, now the system's drivers are loaded and initialized. It is
	// time to load the initial user-space programs and start execution of
	// the actual operating system.

	byte* program;
	size_t programsize;

	// Create an address space for the idle process.
	addr_t idleaddrspace = Memory::Fork();
	if ( !idleaddrspace ) { Panic("could not fork an idle process address space"); }

	// Create an address space for the initial process.
	addr_t initaddrspace = Memory::Fork();
	if ( !initaddrspace ) { Panic("could not fork an initial process address space"); }

	// Create the system idle process.
	Process* idle = new Process;
	if ( !idle ) { Panic("could not allocate idle process"); }
	idle->addrspace = idleaddrspace;
	Memory::SwitchAddressSpace(idleaddrspace);
	Scheduler::SetDummyThreadOwner(idle);
	program = InitRD::Open("idle", &programsize);
	if ( program == NULL ) { PanicF("initrd did not contain 'idle'"); }
	addr_t idlestart = ELF::Construct(idle, program, programsize);
	if ( !idlestart ) { Panic("could not construct ELF image for idle process"); }
	Thread* idlethread = CreateThread(idlestart);
	if ( !idlethread ) { Panic("could not create thread for the idle process"); }
	Scheduler::SetIdleThread(idlethread);

	// Create the initial process.
	Process* init = new Process;
	if ( !init ) { Panic("could not allocate init process"); }
	init->addrspace = initaddrspace;
	Memory::SwitchAddressSpace(initaddrspace);
	Scheduler::SetDummyThreadOwner(init);
	program = InitRD::Open("init", &programsize);
	if ( program == NULL ) { PanicF("initrd did not contain 'init'"); }
	addr_t initstart = ELF::Construct(init, program, programsize);
	if ( !initstart ) { Panic("could not construct ELF image for init process"); }
	Thread* initthread = CreateThread(initstart);
	if ( !initthread ) { Panic("could not create thread for the init process"); }
	Scheduler::SetInitProcess(init);

	// Lastly set up the timer driver and we are ready to run the OS.
	Time::Init();

	// Run the OS.
	Scheduler::MainLoop();
}

} // namespace Sortix
