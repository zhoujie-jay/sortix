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
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/fcache.h>
#include <sortix/kernel/string.h>

#include <sortix/fcntl.h>
#include <sortix/stat.h>
#include <sortix/mman.h>
#include <sortix/wait.h>

#include <assert.h>
#include <errno.h>
#include <malloc.h>

#include "kernelinfo.h"
#include "x86-family/gdt.h"
#include "x86-family/float.h"
#include "multiboot.h"
#include "thread.h"
#include "process.h"
#include "signal.h"
#include "ata.h"
#include "com.h"
#include "uart.h"
#include "logterminal.h"
#include "vgatextbuffer.h"
#include "serialterminal.h"
#include "textterminal.h"
#include "elf.h"
#include "initrd.h"
#include "vga.h"
#include "bga.h"
#include "sound.h"
#include "io.h"
#include "pipe.h"
#include "poll.h"
#include "dispmsg.h"
#include "fs/kram.h"
#include "fs/user.h"
#include "kb/ps2.h"
#include "kb/layout/us.h"

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

static bool TextTermSync(void* user)
{
	return ((TextTerminal*) user)->Sync();
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
	TextBufferHandle textbufhandlestack(NULL, false, &textbuf, false);
	textbufhandle = Ref<TextBufferHandle>(&textbufhandlestack);

	// Setup a text terminal instance.
	TextTerminal textterm(textbufhandle);

	// Register the text terminal as the kernel log and initialize it.
	Log::Init(PrintToTextTerminal, TextTermWidth, TextTermHeight, TextTermSync,
	          &textterm);

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

	initrd = 0;
	initrdsize = 0;

	uint32_t* modules = (uint32_t*) (addr_t) bootinfo->mods_addr;
	for ( uint32_t i = 0; i < bootinfo->mods_count; i++ )
	{
		initrdsize = modules[2*i+1] - modules[2*i+0];
		initrd = (addr_t) modules[2*i+0];
		break;
	}

	if ( !initrd ) { PanicF("No init ramdisk provided"); }

	// Initialize paging and virtual memory.
	Memory::Init(bootinfo);

	// Initialize the GDT and TSS structures.
	GDT::Init();

	// Initialize the interrupt handler table and enable interrupts.
	Interrupt::Init();

	// Initialize the kernel heap.
	_init_heap();

	// Initialize the interrupt worker (before scheduling is enabled).
	Interrupt::InitWorker();

	//
	// Stage 2. Transition to Multithreaded Environment
	//

	// Initialize the process system.
	Process::Init();

	// Initialize the thread system.
	Thread::Init();

	// Initialize Unix Signals.
	Signal::Init();

	// Initialize the scheduler.
	Scheduler::Init();

	// Initialize the Display Message framework.
	DisplayMessage::Init();

	// Now that the base system has been loaded, it's time to go threaded. First
	// we create an object that represents this thread.
	Process* system = new Process;
	if ( !system ) { Panic("Could not allocate the system process"); }
	addr_t systemaddrspace = Memory::GetAddressSpace();
	system->addrspace = systemaddrspace;

	if ( !(system->program_image_path = String::Clone("<kernel process>")) )
		Panic("Unable to clone string for system process name");

	// We construct this thread manually for bootstrap reasons. We wish to
	// create a kernel thread that is the current thread and isn't put into the
	// scheduler's set of runnable threads, but rather run whenever there is
	// _nothing_ else to run on this CPU.
	Thread* idlethread = new Thread;
	idlethread->process = system;
	idlethread->addrspace = idlethread->process->addrspace;
	idlethread->kernelstackpos = (addr_t) stack;
	idlethread->kernelstacksize = STACK_SIZE;
	idlethread->kernelstackmalloced = false;
	idlethread->fpuinitialized = true;
	system->firstthread = idlethread;
	Scheduler::SetIdleThread(idlethread);

	// Let's create a regular kernel thread that can decide what happens next.
	// Note that we don't do the work here: if all other threads are not running
	// and this thread isn't runnable, then there is nothing to run. Therefore
	// we must become the system idle thread.
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
		Panic("Unable to extract initrd into RAM root filesystem.");

	// We no longer need the initrd, so free its resources.
	InitRD::Delete();

	// Get a descriptor for the /dev directory so we can populate it.
	if ( droot->mkdir(&ctx, "dev", 0775) != 0 && errno != EEXIST )
		Panic("Unable to create RAM filesystem /dev directory.");
	Ref<Descriptor> slashdev = droot->open(&ctx, "dev", O_READ | O_DIRECTORY);
	if ( !slashdev )
		Panic("Unable to create descriptor for RAM filesystem /dev directory.");

	//
	// Stage 5. Loading and Initializing Core Drivers.
	//

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

	// Initialize the COM ports.
	COM::Init("/dev", slashdev);

	// Initialize the VGA driver.
	VGA::Init("/dev", slashdev);

	// Initialize the sound driver.
	Sound::Init();

	// Initialize the IO system.
	IO::Init();

	// Initialize the pipe system.
	Pipe::Init();

	// Initialize poll system call.
	Poll::Init();

	// Initialize the kernel information query syscall.
	Info::Init();

	// Initialize the Video Driver framework.
	Video::Init(textbufhandle);

	// Search for PCI devices and load their drivers.
	PCI::Init();

	// Initialize ATA devices.
	ATA::Init("/dev", slashdev);

	// Initialize the BGA driver.
	BGA::Init();

	// Initialize the user-space filesystem framework.
	UserFS::Init("/dev", slashdev);

	//
	// Stage 6. Executing Hosted Environment ("User-Space")
	//
	// Finally, let's transfer control to a new kernel process that will
	// eventually run user-space code known as the operating system.
	addr_t initaddrspace = Memory::Fork();
	if ( !initaddrspace ) { Panic("Could not create init's address space"); }

	Process* init = new Process;
	if ( !init ) { Panic("Could not allocate init process"); }

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

#if defined(PLATFORM_X86)
	#define CPUTYPE_STR "i486-sortix"
#elif defined(PLATFORM_X64)
	#define CPUTYPE_STR "x86_64-sortix"
#else
	#error No cputype environmental variable provided here.
#endif

static void InitThread(void* /*user*/)
{
	// We are the init process's first thread. Let's load the init program from
	// the init ramdisk and transfer execution to it. We will then become a
	// regular user-space program with root permissions.

	Thread* thread = CurrentThread();
	Process* process = CurrentProcess();

	const char* initpath = "/" CPUTYPE_STR "/bin/init";

	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	Ref<Descriptor> root = CurrentProcess()->GetRoot();
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

	const size_t DEFAULT_STACK_SIZE = 512UL * 1024UL;

	size_t stacksize = 0;
	if ( !stacksize ) { stacksize = DEFAULT_STACK_SIZE; }

	addr_t stackpos = process->AllocVirtualAddr(stacksize);
	if ( !stackpos ) { Panic("Could not allocate init stack space"); }

	int prot = PROT_FORK | PROT_READ | PROT_WRITE | PROT_KREAD | PROT_KWRITE;
	if ( !Memory::MapRange(stackpos, stacksize, prot) )
		Panic("Could not allocate init stack memory");

	thread->stackpos = stackpos;
	thread->stacksize = stacksize;

	int argc = 1;
	const char* argv[] = { "init", NULL };
	const char* cputype = "cputype=" CPUTYPE_STR;
	int envc = 1;
	const char* envp[] = { cputype, NULL };
	CPU::InterruptRegisters regs;

	if ( process->Execute(initpath, program, programsize, argc, argv, envc,
	                       envp, &regs) )
		PanicF("Unable to execute %s.", initpath);

	delete[] program;

	// Now become the init process and the operation system shall run.
	CPU::LoadRegisters(&regs);
}

} // namespace Sortix
