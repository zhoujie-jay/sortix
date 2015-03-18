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

    kernel.cpp
    The main kernel initialization routine. Configures hardware and starts an
    initial process from the init ramdisk, allowing a full operating system.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <brand.h>
#include <elf.h>
#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>

#include <sortix/fcntl.h>
#include <sortix/mman.h>
#include <sortix/stat.h>
#include <sortix/wait.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/decl.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/fcache.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/ptable.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/symbol.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/user-timer.h>
#include <sortix/kernel/video.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/worker.h>

#include "ata.h"
#include "com.h"
#include "fs/full.h"
#include "fs/kram.h"
#include "fs/null.h"
#include "fs/zero.h"
#include "gpu/bga/bga.h"
#include "initrd.h"
#include "kb/layout/us.h"
#include "kb/ps2.h"
#include "logterminal.h"
#include "multiboot.h"
#include "net/fs.h"
#include "poll.h"
#include "textterminal.h"
#include "uart.h"
#include "vga.h"
#include "vgatextbuffer.h"

#if defined(__i386__) || defined(__x86_64__)
#include "x86-family/cmos.h"
#include "x86-family/float.h"
#include "x86-family/gdt.h"
#endif

// Keep the stack size aligned with $CPU/boot.s
const size_t STACK_SIZE = 64*1024;
extern "C" { __attribute__((aligned(16))) size_t stack[STACK_SIZE / sizeof(size_t)]; }

namespace Sortix {

void DoWelcome()
{
	Log::Print(BRAND_KERNEL_BOOT_MESSAGE);
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

static void TextTermGetCursor(void* user, size_t* column, size_t* row)
{
	((TextTerminal*) user)->GetCursor(column, row);
}

static bool TextTermSync(void* user)
{
	return ((TextTerminal*) user)->Sync();
}

static bool EmergencyTextTermIsImpaired(void* user)
{
	return ((TextTerminal*) user)->EmergencyIsImpaired();
}

static bool EmergencyTextTermRecoup(void* user)
{
	return ((TextTerminal*) user)->EmergencyRecoup();
}

static void EmergencyTextTermReset(void* user)
{
	((TextTerminal*) user)->EmergencyReset();
}

static
size_t EmergencyPrintToTextTerminal(void* user, const char* str, size_t len)
{
	return ((TextTerminal*) user)->EmergencyPrint(str, len);
}

static size_t EmergencyTextTermWidth(void* user)
{
	return ((TextTerminal*) user)->EmergencyWidth();
}

static size_t EmergencyTextTermHeight(void* user)
{
	return ((TextTerminal*) user)->EmergencyHeight();
}

static void EmergencyTextTermGetCursor(void* user, size_t* column, size_t* row)
{
	((TextTerminal*) user)->EmergencyGetCursor(column, row);
}

static bool EmergencyTextTermSync(void* user)
{
	return ((TextTerminal*) user)->EmergencySync();
}

addr_t initrd;
size_t initrdsize;
Ref<TextBufferHandle> textbufhandle;

extern "C" void KernelInit(unsigned long magic, multiboot_info_t* bootinfo)
{
	(void) magic;

	//
	// Stage 1. Initialization of Early Environment.
	//

	// TODO: Call global constructors using the _init function.

	// Detect available physical memory.
	Memory::Init(bootinfo);

	// Setup a text buffer handle for use by the text terminal.
	uint16_t* const VGAFB = (uint16_t*) 0xB8000;
	const size_t VGA_WIDTH = 80;
	const size_t VGA_HEIGHT = 25;
	static uint16_t vga_attr_buffer[VGA_WIDTH*VGA_HEIGHT];
	static struct TextCharPOD vga_chars_pod_buffer[VGA_WIDTH*VGA_HEIGHT];
	TextChar* vga_chars_buffer = (TextChar*) vga_chars_pod_buffer;
	VGATextBuffer textbuf(VGAFB, vga_chars_buffer, vga_attr_buffer, VGA_WIDTH, VGA_HEIGHT);
	TextBufferHandle textbufhandlestack(NULL, false, &textbuf, false);
	textbufhandle = Ref<TextBufferHandle>(&textbufhandlestack);

	// Setup a text terminal instance.
	TextTerminal textterm(textbufhandle);

	// Register the text terminal as the kernel log.
	Log::device_callback = PrintToTextTerminal;
	Log::device_width = TextTermWidth;
	Log::device_height = TextTermHeight;
	Log::device_get_cursor = TextTermGetCursor;
	Log::device_sync = TextTermSync;
	Log::device_pointer = &textterm;

	// Register the emergency kernel log.
	Log::emergency_device_is_impaired = EmergencyTextTermIsImpaired;
	Log::emergency_device_recoup = EmergencyTextTermRecoup;
	Log::emergency_device_reset = EmergencyTextTermReset;
	Log::emergency_device_callback = EmergencyPrintToTextTerminal;
	Log::emergency_device_width = EmergencyTextTermWidth;
	Log::emergency_device_height = EmergencyTextTermHeight;
	Log::emergency_device_get_cursor = EmergencyTextTermGetCursor;
	Log::emergency_device_sync = EmergencyTextTermSync;
	Log::emergency_device_pointer = &textterm;

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

	initrd = 0;
	initrdsize = 0;

	uint32_t* modules = (uint32_t*) (addr_t) bootinfo->mods_addr;
	for ( uint32_t i = 0; i < bootinfo->mods_count; i++ )
	{
		initrdsize = modules[2*i+1] - modules[2*i+0];
		initrd = (addr_t) modules[2*i+0];
		break;
	}

	if ( !initrd )
		Panic("No init ramdisk provided");

	// Load the kernel symbols if provided by the bootloader.
	do if ( bootinfo->flags & MULTIBOOT_INFO_ELF_SHDR )
	{
		// On i386 and x86_64 we identity map the first 4 MiB memory, if the
		// debugging sections are outside that region, we can't access them
		// directly and we'll have to memory map some physical memory.
		// TODO: Correctly handle the memory being outside 4 MiB. You need to
		//       teach the memory management code to reserve these ranges for
		//       a while until we have used them and add additional complexity
		//       in this code.
		#define BELOW_4MIB(addr, length) ((addr) + (length) <= 4*1024*1024)

		// Find and the verify the section table.
		multiboot_elf_section_header_table_t* elf_sec = &bootinfo->u.elf_sec;
		if ( !BELOW_4MIB(elf_sec->addr, elf_sec->size) )
		{
			Log::PrintF("Warning: the section table was loaded inappropriately by the boot loader, kernel debugging symbols will not be available.\n");
			break;
		}

		#define SECTION(num) ((Elf32_Shdr*) ((uintptr_t) elf_sec->addr + (uintptr_t) elf_sec->size * (uintptr_t) (num)))

		// Verify the section name section.
		Elf32_Shdr* section_string_section = SECTION(elf_sec->shndx);
		if ( !BELOW_4MIB(section_string_section->sh_addr, section_string_section->sh_size) )
		{
			Log::PrintF("Warning: the section string table was loaded inappropriately by the boot loader, kernel debugging symbols will not be available.\n");
			break;
		}

		if ( !section_string_section )
			break;

		const char* section_string_table = (const char*) (uintptr_t) section_string_section->sh_addr;

		// Find the symbol table.
		Elf32_Shdr* symbol_table_section = NULL;
		for ( unsigned i = 0; i < elf_sec->num && !symbol_table_section; i++ )
		{
			Elf32_Shdr* section = SECTION(i);
			if ( !strcmp(section_string_table + section->sh_name, ".symtab") )
				symbol_table_section = section;
		}

		if ( !symbol_table_section )
			break;

		if ( !BELOW_4MIB(symbol_table_section->sh_addr, symbol_table_section->sh_size) )
		{
			Log::PrintF("Warning: the symbol table was loaded inappropriately by the boot loader, kernel debugging symbols will not be available.\n");
			break;
		}

		// Find the symbol string table.
		Elf32_Shdr* string_table_section = NULL;
		for ( unsigned i = 0; i < elf_sec->num && !string_table_section; i++ )
		{
			Elf32_Shdr* section = SECTION(i);
			if ( !strcmp(section_string_table + section->sh_name, ".strtab") )
				string_table_section = section;
		}

		if ( !string_table_section )
			break;

		if ( !BELOW_4MIB(string_table_section->sh_addr, string_table_section->sh_size) )
		{
			Log::PrintF("Warning: the symbol string table was loaded inappropriately by the boot loader, kernel debugging symbols will not be available.\n");
			break;
		}

		// Duplicate the data structures and convert them to the kernel symbol
		// table format and register it for later debugging.
		const char* elf_string_table = (const char*) (uintptr_t) string_table_section->sh_addr;
		size_t elf_string_table_size = string_table_section->sh_size;
		Elf32_Sym* elf_symbols = (Elf32_Sym*) (uintptr_t) symbol_table_section->sh_addr;
		size_t elf_symbol_count = symbol_table_section->sh_size / sizeof(Elf32_Sym);

		if ( !elf_symbol_count || elf_symbol_count == 1 /* null symbol */)
			break;

		char* string_table = new char[elf_string_table_size];
		if ( !string_table )
		{
			Log::PrintF("Warning: unable to allocate the kernel symbol string table, kernel debugging symbols will not be available.\n");
			break;
		}
		memcpy(string_table, elf_string_table, elf_string_table_size);

		Symbol* symbols = new Symbol[elf_symbol_count-1];
		if ( !symbols )
		{
			Log::PrintF("Warning: unable to allocate the kernel symbol table, kernel debugging symbols will not be available.\n");
			delete[] string_table;
			break;
		}

		// Copy all entires except the leading null entry.
		for ( size_t i = 1; i < elf_symbol_count; i++ )
		{
			symbols[i-1].address = elf_symbols[i].st_value;
			symbols[i-1].size = elf_symbols[i].st_size;
			symbols[i-1].name = string_table + elf_symbols[i].st_name;
		}

		SetKernelSymbolTable(symbols, elf_symbol_count-1);
	} while ( false );

	// Initialize the interrupt handler table and enable interrupts.
	Interrupt::Init();

	// Initialize the interrupt worker (before scheduling is enabled).
	Interrupt::InitWorker();

	//
	// Stage 2. Transition to Multithreaded Environment
	//

	// Initialize the clocks.
	Time::Init();

	// Initialize Unix Signals.
	Signal::Init();

	// Initialize the scheduler.
	Scheduler::Init();

	// Now that the base system has been loaded, it's time to go threaded. First
	// we create an object that represents this process.
	Ref<ProcessTable> ptable(new ProcessTable());
	if ( !ptable )
		Panic("Could not allocate the process table");
	Process* system = new Process;
	if ( !system )
		Panic("Could not allocate the system process");
	if ( (system->pid = (system->ptable = ptable)->Allocate(system)) < 0 )
		Panic("Could not allocate the system process a pid");
	ptable.Reset();
	system->addrspace = Memory::GetAddressSpace();
	system->group = system;
	system->groupprev = NULL;
	system->groupnext = NULL;
	system->groupfirst = system;

	if ( !(system->program_image_path = String::Clone("<kernel process>")) )
		Panic("Unable to clone string for system process name");

	// We construct this thread manually for bootstrap reasons. We wish to
	// create a kernel thread that is the current thread and isn't put into the
	// scheduler's set of runnable threads, but rather run whenever there is
	// _nothing_ else to run on this CPU.
	Thread* idlethread = AllocateThread();
	idlethread->process = system;
	idlethread->kernelstackpos = (addr_t) stack;
	idlethread->kernelstacksize = STACK_SIZE;
	idlethread->kernelstackmalloced = false;
	system->firstthread = idlethread;
	Scheduler::SetIdleThread(idlethread);

	// Let's create a regular kernel thread that can decide what happens next.
	// Note that we don't do the work here: if all other threads are not running
	// and this thread isn't runnable, then there is nothing to run. Therefore
	// we must become the system idle thread.
	RunKernelThread(BootThread, NULL);

	// The time driver will run the scheduler on the next timer interrupt.
	Time::Start();

	// Become the system idle thread.
	SystemIdleThread(NULL);
}

static void SystemIdleThread(void* /*user*/)
{
	// Alright, we are now the system idle thread. If there is nothing to do,
	// then we are run. Note that we must never do any real work here as the
	// idle thread must always be runnable.
	while(true);
}

static void BootThread(void* /*user*/)
{
	//
	// Stage 3. Spawning Kernel Worker Threads.
	//

	// Hello, threaded world! You can now regard the kernel as a multi-threaded
	// process with super-root access to the system. Before we boot the full
	// system we need to start some worker threads.
	// Let's create the interrupt worker thread that executes additional work
	// requested by interrupt handlers, where such work isn't safe.
	Thread* interruptworker = RunKernelThread(Interrupt::WorkerThread, NULL);
	if ( !interruptworker )
		Panic("Could not create interrupt worker");

	// Initialize the worker thread data structures.
	Worker::Init();

	// Create a general purpose worker thread.
	Thread* workerthread = RunKernelThread(Worker::Thread, NULL);
	if ( !workerthread )
		Panic("Unable to create general purpose worker thread");

	//
	// Stage 4. Initialize the Filesystem
	//

	// Bring up the filesystem cache.
	FileCache::Init();

	Ref<DescriptorTable> dtable(new DescriptorTable());
	if ( !dtable )
		Panic("Unable to allocate descriptor table");
	Ref<MountTable> mtable(new MountTable());
	if ( !mtable )
		Panic("Unable to allocate mount table.");
	CurrentProcess()->BootstrapTables(dtable, mtable);

	// Let's begin preparing the filesystem.
	// TODO: Setup the right device id for the KRAMFS dir?
	Ref<Inode> iroot(new KRAMFS::Dir((dev_t) 0, (ino_t) 0, 0, 0, 0755));
	if ( !iroot )
		Panic("Unable to allocate root inode.");
	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	Ref<Vnode> vroot(new Vnode(iroot, Ref<Vnode>(NULL), 0, 0));
	if ( !vroot )
		Panic("Unable to allocate root vnode.");
	Ref<Descriptor> droot(new Descriptor(vroot, O_SEARCH));
	if ( !droot )
		Panic("Unable to allocate root descriptor.");
	CurrentProcess()->BootstrapDirectories(droot);

	// Initialize the root directory.
	if ( iroot->link_raw(&ctx, ".", iroot) != 0 )
		Panic("Unable to link /. to /");
	if ( iroot->link_raw(&ctx, "..", iroot) != 0 )
		Panic("Unable to link /.. to /");

	// Install the initrd into our fresh RAM filesystem.
	if ( !InitRD::ExtractFromPhysicalInto(initrd, initrdsize, droot) )
		Panic("Unable to extract initrd into RAM root filesystem. Your machine "
		      "needs more memory to boot using this initrd, as a rule of thumb "
		      "you need twice as much memory as the size of the initrd device.");

	//
	// Stage 5. Loading and Initializing Core Drivers.
	//

	// Initialize the real-time clock.
	CMOS::Init();

	// Get a descriptor for the /dev directory so we can populate it.
	if ( droot->mkdir(&ctx, "dev", 0775) != 0 && errno != EEXIST )
		Panic("Unable to create RAM filesystem /dev directory.");
	Ref<Descriptor> slashdev = droot->open(&ctx, "dev", O_READ | O_DIRECTORY);
	if ( !slashdev )
		Panic("Unable to create descriptor for RAM filesystem /dev directory.");

	// Initialize the keyboard.
	Keyboard* keyboard = new PS2Keyboard(0x60, Interrupt::IRQ1);
	if ( !keyboard )
		Panic("Could not allocate PS2 Keyboard driver");
	KeyboardLayout* kblayout = new KBLayoutUS;
	if ( !kblayout )
		Panic("Could not allocate keyboard layout driver");

	// Register the kernel terminal as /dev/tty.
	Ref<Inode> tty(new LogTerminal(slashdev->dev, 0666, 0, 0, keyboard, kblayout));
	if ( !tty )
		Panic("Could not allocate a kernel terminal");
	if ( LinkInodeInDir(&ctx, slashdev, "tty", tty) != 0 )
		Panic("Unable to link /dev/tty to kernel terminal.");

	// Register the null device as /dev/null.
	Ref<Inode> null_device(new Null(slashdev->dev, (ino_t) 0, (uid_t) 0,
	                                (gid_t) 0, (mode_t) 0666));
	if ( !null_device )
		Panic("Could not allocate a null device");
	if ( LinkInodeInDir(&ctx, slashdev, "null", null_device) != 0 )
		Panic("Unable to link /dev/null to the null device.");

	// Register the zero device as /dev/zero.
	Ref<Inode> zero_device(new Zero(slashdev->dev, (ino_t) 0, (uid_t) 0,
	                                (gid_t) 0, (mode_t) 0666));
	if ( !zero_device )
		Panic("Could not allocate a zero device");
	if ( LinkInodeInDir(&ctx, slashdev, "zero", zero_device) != 0 )
		Panic("Unable to link /dev/zero to the zero device.");

	// Register the full device as /dev/full.
	Ref<Inode> full_device(new Full(slashdev->dev, (ino_t) 0, (uid_t) 0,
	                                (gid_t) 0, (mode_t) 0666));
	if ( !full_device )
		Panic("Could not allocate a full device");
	if ( LinkInodeInDir(&ctx, slashdev, "full", full_device) != 0 )
		Panic("Unable to link /dev/full to the full device.");

	// Initialize the COM ports.
	COM::Init("/dev", slashdev);

	// Initialize the VGA driver.
	VGA::Init("/dev", slashdev);

	// Initialize the Video Driver framework.
	Video::Init(textbufhandle);

	// Search for PCI devices and load their drivers.
	PCI::Init();

	// Initialize ATA devices.
	ATA::Init("/dev", slashdev);

	// Initialize the BGA driver.
	BGA::Init();

	// Initialize the filesystem network-
	NetFS::Init("/dev", slashdev);

	//
	// Stage 6. Executing Hosted Environment ("User-Space")
	//
	// Finally, let's transfer control to a new kernel process that will
	// eventually run user-space code known as the operating system.
	addr_t initaddrspace = Memory::Fork();
	if ( !initaddrspace ) { Panic("Could not create init's address space"); }

	Process* init = new Process;
	if ( !init )
		Panic("Could not allocate init process");
	if ( (init->pid = (init->ptable = CurrentProcess()->ptable)->Allocate(init)) < 0 )
		Panic("Could not allocate init a pid");
	init->group = init;
	init->groupprev = NULL;
	init->groupnext = NULL;
	init->groupfirst = init;

	CurrentProcess()->AddChildProcess(init);

	// TODO: Why don't we fork from pid=0 and this is done for us?
	// TODO: Fork dtable and mtable, don't share them!
	init->BootstrapTables(dtable, mtable);
	dtable.Reset();
	mtable.Reset();
	init->BootstrapDirectories(droot);
	init->addrspace = initaddrspace;
	Scheduler::SetInitProcess(init);

	Thread* initthread = RunKernelThread(init, InitThread, NULL);
	if ( !initthread )
		Panic("Could not create init thread");

	// Wait until init init is done and then shut down the computer.
	int status;
	pid_t pid = CurrentProcess()->Wait(init->pid, &status, 0);
	if ( pid != init->pid )
		PanicF("Waiting for init to exit returned %ji (errno=%i)", (intmax_t) pid, errno);

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

	Process* process = CurrentProcess();

	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	Ref<Descriptor> root = CurrentProcess()->GetRoot();

	Ref<DescriptorTable> dtable = process->GetDTable();

	Ref<Descriptor> tty_stdin = root->open(&ctx, "/dev/tty", O_READ);
	if ( !tty_stdin || dtable->Allocate(tty_stdin, 0) != 0 )
		Panic("Could not prepare stdin for initialization process");
	Ref<Descriptor> tty_stdout = root->open(&ctx, "/dev/tty", O_WRITE);
	if ( !tty_stdout || dtable->Allocate(tty_stdout, 0) != 1 )
		Panic("Could not prepare stdout for initialization process");
	Ref<Descriptor> tty_stderr = root->open(&ctx, "/dev/tty", O_WRITE);
	if ( !tty_stderr || dtable->Allocate(tty_stderr, 0) != 2 )
		Panic("Could not prepare stderr for initialization process");

	dtable.Reset();

	const char* initpath = "/bin/init";
	Ref<Descriptor> init = root->open(&ctx, initpath, O_EXEC | O_READ);
	if ( !init )
		PanicF("Could not open %s in early kernel RAM filesystem:\n%s",
		       initpath, strerror(errno));
	struct stat st;
	if ( init->stat(&ctx, &st) )
		PanicF("Could not stat '%s' in initrd.", initpath);
	assert(0 <= st.st_size);
	if ( (uintmax_t) SIZE_MAX < (uintmax_t) st.st_size )
		PanicF("%s is bigger than SIZE_MAX.", initpath);
	size_t programsize = st.st_size;
	uint8_t* program = new uint8_t[programsize];
	if ( !program )
		PanicF("Unable to allocate 0x%zx bytes needed for %s.", programsize, initpath);
	size_t sofar = 0;
	while ( sofar < programsize )
	{
		ssize_t numbytes = init->read(&ctx, program+sofar, programsize-sofar);
		if ( !numbytes )
			PanicF("Premature EOF when reading %s.", initpath);
		if ( numbytes < 0 )
			PanicF("IO error when reading %s.", initpath);
		sofar += numbytes;
	}

	init.Reset();

	int argc = 1;
	const char* argv[] = { "init", NULL };
	int envc = 0;
	const char* envp[] = { NULL };
	struct thread_registers regs;
	assert((((uintptr_t) &regs) & (alignof(regs)-1)) == 0);

	if ( process->Execute(initpath, program, programsize, argc, argv, envc,
	                       envp, &regs) )
		PanicF("Unable to execute %s.", initpath);

	delete[] program;

	// Now become the init process and the operation system shall run.
	LoadRegisters(&regs);
}

} // namespace Sortix
