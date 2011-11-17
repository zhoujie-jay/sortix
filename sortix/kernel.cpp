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

	kernel.cpp
	A common interface shared by all devices that can be printed text to.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include <libmaxsi/string.h>
#include <libmaxsi/format.h>
#include "log.h"
#include "panic.h"
#include "descriptor_tables.h"
#include "time.h"
#include "keyboard.h"
#include "multiboot.h"
#include "memorymanagement.h"
#include "thread.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"
#include "pci.h"
#include "uart.h"
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

using namespace Maxsi;

void* RunApplication(void* Parameter);

extern "C" size_t stack[4096] = {0};
extern "C" size_t stackend = 0;

namespace Sortix
{
#ifdef PLATFORM_HTTP
	namespace HTTP { void Init(); }
#endif

	void DoBSoD()
	{
#ifdef PLATFORM_SERIAL
		UART::WriteChar(27);
		UART::WriteChar(91);
		UART::WriteChar(48 + 4);
		UART::WriteChar(48 + 4);
		UART::WriteChar(109);
#endif

		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("Windows Boot Manager has experienced a problem.                                 ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("    Status: 0xc000000f                                                          ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("    Info: An error occured during transferring execution.                       ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("You can try to recover the system with the Microsoft Windows System Recovery    ");
		Log::Print("Tools. (You might need to restart the system manually).                         ");
		Log::Print("                                                                                ");
		Log::Print("If the problem continues, please contact your system administrator or computer  ");
		Log::Print("manufacturer.                                                                   ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");
		Log::Print("                                                                                ");

#ifdef JSSORTIX
		JSSortix::Exit();
#else
		while ( true ) { }
#endif
	}

	void DoMaxsiLogo()
	{
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
		Log::Print("                           BOOTING OPERATING SYSTEM...                          ");
	}

	void DoWelcome()
	{
#ifdef BSOD
		DoBSoD();
#endif

		DoMaxsiLogo();
	}

	extern "C" void KernelInit(unsigned long Magic, multiboot_info_t* BootInfo)
	{
#ifdef JSSORTIX
		// TODO: Make JSVM multiboot compliant.
		multiboot_info_t MBInfo; BootInfo = &MBInfo;
		multiboot_memory_map_t MBMMap;

		MBMMap.addr = 0x100000;
		MBMMap.len = 0xC00000;
		MBMMap.type = MULTIBOOT_MEMORY_AVAILABLE;
		MBMMap.size = sizeof(MBMMap) - sizeof(MBMMap.size);
		BootInfo->flags = MULTIBOOT_INFO_MEM_MAP;
		BootInfo->mmap_addr = (multiboot_uint32_t) &MBMMap;
		BootInfo->mmap_length = sizeof(MBMMap);
#endif

		// Initialize system calls.
		Syscall::Init();

		// Initialize the default terminal.
		Maxsi::Format::Callback logcallback;
		void* logpointer;

#if PLATFORM_SERIAL
		// Initialize the serial driver.
		UART::Init();

		SerialTerminal::Init();
		logcallback = SerialTerminal::Print;
		logpointer = NULL;

#else
		VGATerminal::Init();
		logcallback = VGATerminal::Print;
		logpointer = NULL;

#endif

		// Initialize the kernel log.
		Log::Init(logcallback, logpointer);

		// Just a test to see if the color system works! Make the BG red!
		Log::Print("\e[37;41m\e[2J");

		// Display the boot welcome screen.
		DoWelcome();

#ifndef JSSORTIX
		// Search for PCI devices and load their drivers.
		PCI::Init();
#endif

		// Initialize the paging and virtual memory.
		Memory::Init(BootInfo);

		uint8_t* initrd = NULL;
		size_t initrdsize = 0;

#ifndef JSSORTIX
		uint8_t** modules = (uint8_t**) BootInfo->mods_addr;
		for ( uint32_t I = 0; I < BootInfo->mods_count; I++ )
		{
			initrdsize = modules[2*I+1] - modules[2*I+0];
			initrd = modules[2*I+0];
			break;
		}
#else
		// TODO: UGLY HACK because JSVM doesn't support multiboot yet!
		initrd = (uint8_t*) 0x180000UL;
		initrdsize = 0x80000; // 512 KiB
#endif

		if ( initrd == NULL ) { PanicF("No initrd provided"); }

		// Initialize the GDT and TSS structures.
		GDT::Init();

#ifdef PLATFORM_X64
		Log::Print("Halt: CPU X64 cannot boot because interrupts are not yet "
		           "supported under 64-bit Sortix.\n");
		Log::Print("Sorry, it simply isn't possible to fully boot Sortix in x64 mode yet.\n");
		Log::Print("X64 may be working when Sortix 0.5 comes out, or try the git master.\n");
		while(true);
#endif

		// Initialize the interrupt descriptor tables.
		IDT::Init();

		if ( BootInfo == NULL ) { Panic("kernel.cpp: The bootinfo structure was NULL. Are your bootloader multiboot compliant?"); }

		// Initialize the kernel heap.
		Maxsi::Memory::Init();

		// Initialize the keyboard.
		Keyboard::Init();

		// Initialize the VGA driver.
		VGA::Init();

		// Initialize the sound driver.
		Sound::Init();

		// Initialize the process system.
		Process::Init();

		// Initialize the IO system.
		IO::Init();

		// Initialize the pipe system.
		Pipe::Init();

		// Initialize the filesystem system.
		FileSystem::Init();

		// Initialize the mount system.
		Mount::Init();

		// Initialize the scheduler.
		Scheduler::Init();

		// Set up the initial ram disk.
		InitRD::Init(initrd, initrdsize);

		// Alright, now the system's drivers are loaded and initialized. It is
		// time to load the initial user-space programs and start execution of
		// the actual operating system.

		byte* program;
		size_t programsize;

		// Create an address space for the idle process.
		addr_t idleaddrspace = Memory::Fork();
		if ( !idleaddrspace ) { Panic("could not fork an idle process"); }

		// Create an address space for the initial process.
		addr_t initaddrspace = Memory::Fork();
		if ( !initaddrspace ) { Panic("could not fork an initial process"); }

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
}
