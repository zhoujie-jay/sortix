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

	scheduler.h
	Handles the creation and management of threads.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "panic.h"
#include "scheduler.h"
#include "globals.h"
#include "multiboot.h"
#include "memorymanagement.h"
#include "descriptor_tables.h"

#include "iprintable.h"
#include "log.h"

#include "sound.h" // HACK

namespace Sortix
{
	const bool LOG_SWITCHING = false;

	namespace Scheduler
	{
		// This is a very small thread that does absoluting nothing!
		char NoopThreadData[sizeof(Thread)];
		Thread* NoopThread;
		const size_t NoopThreadStackLength = 8;
		size_t NoopThreadStack[NoopThreadStackLength];
		void NoopFunction() { while ( true ) { } }

		// Linked lists that implements a very simple scheduler.
		Thread* currentThread;
		Thread* firstRunnableThread;
		Thread* firstUnrunnableThread;
		Thread* firstSleeping;
		size_t AllocatedThreadId;
	}

	Thread::Thread(Process* process, size_t id, addr_t stack, size_t stackLength)
	{
		ASSERT(process != NULL);
		ASSERT(process->IsSane());

		_process = process;
		_id = id;
		_stack = stack;
		_stackLength = stackLength;
		_state = INFANT;
		_prevThread = this;
		_nextThread = this;
		_inThisList = NULL;
		_nextSleepingThread = NULL;
	}

	Thread::~Thread()
	{
		Unlink();

		Page::Put(_stack);
	}

	void Thread::Unlink()
	{
		if ( _inThisList != NULL && *_inThisList == this )
		{
			*_inThisList = ( _nextThread != this ) ? _nextThread : NULL;
			_inThisList = NULL;
		}
		if ( _nextThread != this ) { _prevThread->_nextThread = _nextThread; _nextThread->_prevThread = _prevThread; _prevThread = this; _nextThread = this; }
	}
	
	void Thread::Relink(Thread** list)
	{
		Unlink();

		_inThisList = list;	
		if ( *list != NULL )
		{
			(*list)->_prevThread->_nextThread = this;
			_prevThread = (*list)->_prevThread;
			(*list)->_prevThread = this;
			_nextThread = *list;
		}
		else
		{
			*list = this;
		}
	}

	void Thread::SetState(State newState)
	{
		if ( newState == _state ) { return; }

		if ( newState == RUNNABLE )
		{
			Relink(&Scheduler::firstRunnableThread);
		}
		else if ( ( newState == UNRUNNABLE ) /*&& ( State != UNRUNNABLE || State != WAITING )*/ )
		{
			Relink(&Scheduler::firstUnrunnableThread);
		}

		_state = newState;
	}

	void Thread::SaveRegisters(CPU::InterruptRegisters* Src)
	{
#ifdef PLATFORM_X86
		_registers.eip = Src->eip; _registers.useresp = Src->useresp; _registers.eax = Src->eax; _registers.ebx = Src->ebx; _registers.ecx = Src->ecx; _registers.edx = Src->edx; _registers.edi = Src->edi; _registers.esi = Src->esi; _registers.ebp = Src->ebp;
#else
		#warning "No threads are available on this arch"
		while(true);
#endif
	}

	void Thread::LoadRegisters(CPU::InterruptRegisters* Dest)
	{
#ifdef PLATFORM_X86
		Dest->eip = _registers.eip; Dest->useresp = _registers.useresp; Dest->eax = _registers.eax; Dest->ebx = _registers.ebx; Dest->ecx = _registers.ecx; Dest->edx = _registers.edx; Dest->edi = _registers.edi; Dest->esi = _registers.esi; Dest->ebp = _registers.ebp;
#else
		#warning "No threads are available on this arch"
		while(true);
#endif
	}

	void Thread::Sleep(uintmax_t miliseconds)
	{
		if ( miliseconds == 0 ) { return; }

		_nextSleepingThread = NULL;
		Thread* Thread = Scheduler::firstSleeping;

		if ( Thread == NULL )
		{
			Scheduler::firstSleeping = this;
		}
		else if ( miliseconds < Thread->_sleepMilisecondsLeft )
		{
			Scheduler::firstSleeping = this;
			_nextSleepingThread = Thread;
			Thread->_sleepMilisecondsLeft -= miliseconds;
		}
		else
		{
			while ( true )
			{
				if ( Thread->_nextSleepingThread == NULL )
				{
					Thread->_nextSleepingThread = this; break;
				}
				else if ( miliseconds < Thread->_nextSleepingThread->_sleepMilisecondsLeft )
				{
					Thread->_nextSleepingThread->_sleepMilisecondsLeft -= miliseconds;
					_nextSleepingThread = Thread->_nextSleepingThread;
					Thread->_nextSleepingThread = this; break;
				}
				else
				{
					miliseconds -= Thread->_sleepMilisecondsLeft;
					Thread = Thread->_nextSleepingThread;
				}
			}
		}

		_sleepMilisecondsLeft = miliseconds;
		SetState(UNRUNNABLE);	
	}

	bool sigintpending = false;
	void SigInt() { sigintpending = true; }

	namespace Scheduler
	{
		// Initializes the scheduling subsystem.
		void Init()
		{
			sigintpending = false;

			currentThread = NULL;
			firstRunnableThread = NULL;
			firstUnrunnableThread = NULL;
			firstSleeping = NULL;
			AllocatedThreadId = 1;

			// Create an address space for the idle process.
			addr_t noopaddrspace = VirtualMemory::CreateAddressSpace();

			// Create the noop process.
			Process* noopprocess = new Process(noopaddrspace);

			// Initialize the thread that does nothing.
			NoopThread = new ((void*) NoopThreadData) Thread(noopprocess, 0, NULL, 0);
			NoopThread->SetState(Thread::State::NOOP);
#ifdef PLATFORM_X86
			NoopThread->_registers.useresp = (uint32_t) NoopThreadStack + NoopThreadStackLength;
			NoopThread->_registers.ebp = (uint32_t) NoopThreadStack + NoopThreadStackLength;
			NoopThread->_registers.eip = (uint32_t) &NoopFunction; // NoopFunction;
#else
			#warning "No scheduler are available on this arch"
			while(true);
#endif

			// Allocate and set up a stack for the kernel to use during interrupts.
			addr_t KernelStackPage = Page::Get();
			if ( KernelStackPage == 0 ) { Panic("scheduler.cpp: could not allocate kernel interrupt stack for tss!"); }

#ifdef PLATFORM_VIRTUAL_MEMORY
			uintptr_t MapTo = 0x80000000;

			VirtualMemory::MapKernel(MapTo, (uintptr_t) KernelStackPage);
#endif

			GDT::SetKernelStack((size_t*) (MapTo+4096));
		}

		// Once the init process is spawned and IRQ0 is enabled, this process
		// simply awaits an IRQ0 and then we shall be scheduling.
		void MainLoop()
		{
			Log::PrintF("Waiting for IRQ0\n");
			// Simply wait for the next IRQ0 and then the OS will run.
			while ( true ) { }
		}

		Thread* CreateThread(Process* Process, Thread::Entry Start, void* Parameter1, void* Parameter2, size_t StackSize)
		{
			ASSERT(Process != NULL);
			ASSERT(Process->IsSane());
			ASSERT(Start != NULL);

			// The current default stack size is 4096 bytes.
			if ( StackSize == SIZE_MAX ) { StackSize = 4096; }

			// TODO: We only support stacks of up to one page!
			if ( 4096 < StackSize ) { StackSize = 4096; }

#ifndef PLATFORM_KERNEL_HEAP
			// TODO: Use the proper memory management systems using new and delete instead of these hacks!
			// TODO: These allocations might NOT be thread safe!
			void* ThreadPage = Page::Get();
			if ( ThreadPage == NULL ) { return NULL; }
#endif

			// Allocate a stack for this thread.
			size_t StackLength = StackSize / sizeof(size_t);
			addr_t PhysStack = Page::Get();
			if ( PhysStack == 0 )
			{
#ifndef PLATFORM_KERNEL_HEAP
				Page::Put(ThreadPage);
#endif
				return NULL;
			}

			// Create a new thread data structure.
			Thread* thread = new
#ifndef PLATFORM_KERNEL_HEAP
			                     (ThreadPage)
#endif
                                              Thread(Process, AllocatedThreadId++, PhysStack, StackLength);

#ifdef PLATFORM_X86

#ifdef PLATFORM_VIRTUAL_MEMORY
			uintptr_t StackPos = 0x80000000UL;
			uintptr_t MapTo = StackPos - 4096UL;

			addr_t OldAddrSpace = VirtualMemory::SwitchAddressSpace(Process->GetAddressSpace());

			VirtualMemory::MapUser(MapTo, PhysStack);
#else
			uintptr_t StackPos = (uintptr_t) PhysStack + 4096;
#endif
			size_t* Stack = (size_t*) StackPos;

#ifdef PLATFORM_X86
			// Prepare the parameters for the entry function (C calling convention).
			//Stack[StackLength - 1] = (size_t) 0xFACE; // Parameter2
			Stack[-1] = (size_t) Parameter2; // Parameter2
			Stack[-2] = (size_t) Parameter1; // Parameter1
			Stack[-3] = (size_t) thread->GetId(); // This thread's id.
			Stack[-4] = (size_t) 0x0; // Eip
			thread->_registers.ebp = thread->_registers.useresp = (uint32_t) (StackPos - 4*sizeof(size_t)); // Point to the last word used on the stack.
			thread->_registers.eip = (uint32_t) Start; // Point to our entry function.
#endif

#else
			#warning "No threads are available on this arch"
			while(true);
#endif

			// Mark the thread as running, which adds it to the scheduler's linked list.
			thread->SetState(Thread::State::RUNNABLE);

#ifdef PLATFORM_VIRTUAL_MEMORY
			// Avoid side effects by restoring the old address space.
			VirtualMemory::SwitchAddressSpace(OldAddrSpace);
#endif

			return thread;
		}

		Thread* PopNextThread()
		{
			//Log::PrintF("PopNextThread(): currentThread = 0x%p, firstRunnableThread = 0x%p, firstRunnableThread->PrevThread = 0x%p, firstRunnableThread->NextThread = 0x%p\n", currentThread, firstRunnableThread, firstRunnableThread->PrevThread, firstRunnableThread->NextThread);

			if ( firstRunnableThread == NULL )
			{
				return NoopThread;
			}
			else if ( currentThread == firstRunnableThread )
			{
				firstRunnableThread = firstRunnableThread->_nextThread;
			}

			return firstRunnableThread;
		}

		void Switch(CPU::InterruptRegisters* R, uintmax_t TimePassed)
		{
			//Log::PrintF("Scheduling while at eip=0x%p...", R->eip);
			//Log::Print("\n");

			if ( currentThread != NoopThread && currentThread->GetProcess() && sigintpending )
			{
				Sound::Mute();
				const char* programname = "sh";
				R->ebx = (uint32_t) programname;
				SysExecute(R);
				sigintpending = false;
				Log::Print("^C\n");
			}

			WakeSleeping(TimePassed);

			// Find the next thread to be run.
			Thread* NextThread = PopNextThread();

			//if ( NextThread == NoopThread ) { PanicF("Going to NoopThread! Noop=0x%p, First=0x%p -> 0x%p, Unrun=0x%p", NoopThread, firstRunnableThread, firstRunnableThread->NextThread, firstUnrunnableThread); }

			// If the next thread happens to be the current one, simply do nothing.
			if ( currentThread != NextThread )
			{
				// Save the hardware registers of the current thread.
				if ( currentThread != NULL )
				{
					currentThread->SaveRegisters(R);
				}

				if ( LOG_SWITCHING && NextThread != NoopThread )
				{
					Log::PrintF("Switching from thread at 0x%p to thread at 0x%p\n", currentThread);
				}

				// If applicable, switch the virtual address space.
				VirtualMemory::SwitchAddressSpace(NextThread->GetProcess()->GetAddressSpace());

				currentThread = NextThread;

				// Load the hardware registers of the next thread.
				//Log::PrintF("Switching to thread at 0x%p\n", NextThread);
				NextThread->LoadRegisters(R);
			}
			else
			{
				if ( LOG_SWITCHING )
				{
					//Log::PrintF("Staying in thread 0x%p\n", currentThread);
				}
			}

#ifdef PLATFORM_X86

			if ( currentThread != NoopThread )
			{
				uint32_t RPL = 0x3;

				// Jump into user-space!
				R->ds = 0x20 | RPL; // Set the data segment and Requested Privilege Level.
				R->cs = 0x18 | RPL; // Set the code segment and Requested Privilege Level.
				R->ss = 0x20 | RPL; // Set the stack segment and Requested Privilege Level.
			}
			else
			{
				uint32_t RPL = 0x0;

				// Jump into kernel-space!
				R->ds = 0x10 | RPL; // Set the data segment and Requested Privilege Level.
				R->cs = 0x08 | RPL; // Set the code segment and Requested Privilege Level.
				R->ss = 0x10 | RPL; // Set the stack segment and Requested Privilege Level.
			}

			R->eflags |= 0x200; // Enable the enable interrupts flag in EFLAGS!
#else
			#warning "No threads are available on this arch"
			while(true);
#endif

			//Log::PrintF("ds=0x%x, edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x, ebx=0x%x, edx=0x%x, ecx=0x%x, eax=0x%x, int_no=0x%x, err_code=0x%x, eip=0x%x, cs=0x%x, eflags=0x%x, useresp=0x%x, ss=0x%x\n", R->ds, R->edi, R->esi, R->ebp, R->esp, R->ebx, R->edx, R->ecx, R->eax, R->int_no, R->err_code, R->eip, R->cs, R->eflags, R->useresp, R->ss);

#if 0
			size_t* Stack = (size_t*) R->useresp;


			// TODO: HACK: Currently ESP is not properly set after we return
			// from the interrupt. So we call a helper function that restores
			// it by storing it in EAX, then restoring EAX, and restoring EIP,
			// then we should have returned successfully.
			
			Stack[-1] = (size_t) &PrintRegistersAndDie; // R->eip;
			Stack[-2] = (size_t) R->eax;
			R->eax = (uint32_t) (Stack - 2);
			R->eip = (uint32_t) &RestoreStack;

			Log::PrintF("ds=0x%x, edi=0x%x, esi=0x%x, ebp=0x%x, esp=0x%x, ebx=0x%x, edx=0x%x, ecx=0x%x, eax=0x%x, int_no=0x%x, err_code=0x%x, eip=0x%x, cs=0x%x, eflags=0x%x, useresp=0x%x, ss=0x%x\n", R->ds, R->edi, R->esi, R->ebp, R->esp, R->ebx, R->edx, R->ecx, R->eax, R->int_no, R->err_code, R->eip, R->cs, R->eflags, R->useresp, R->ss);
#endif

			//Log::PrintF("Stack = {0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p}\n", Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7], Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

			//Log::PrintF(" resuming at eip=0x%p\n", R->eip);
		}

		void WakeSleeping(uintmax_t TimePassed)
		{
			while ( firstSleeping != NULL )
			{
				if ( TimePassed < firstSleeping->_sleepMilisecondsLeft ) { firstSleeping->_sleepMilisecondsLeft -= TimePassed; break; }

				TimePassed -= firstSleeping->_sleepMilisecondsLeft;
				firstSleeping->_sleepMilisecondsLeft = 0;
				firstSleeping->SetState(Thread::State::RUNNABLE);
				Thread* Next = firstSleeping->_nextSleepingThread;
				firstSleeping->_nextSleepingThread = NULL;
				firstSleeping = Next;		
			}
		}

		void ExitThread(Thread* Thread, void* Result)
		{
			//Log::PrintF("<ExitedThread debug=\"1\" thread=\"%p\"/>\n", Thread);
			// TODO: What do we do with the result parameter?
			Thread->~Thread();
			//Log::PrintF("<ExitedThread debug=\"2\" thread=\"%p\"/>\n", Thread);
#ifndef PLATFORM_KERNEL_HEAP
			Page::Put((addr_t) Thread);
#else
			delete Thread;
#endif
			//Log::PrintF("<ExitedThread debug=\"3\" thread=\"%p\"/>\n", Thread);

			if ( Thread == currentThread ) { currentThread = NULL; }
		}


		#define ASSERDDD(invariant) \
			if ( unlikely(!(invariant)) ) \
			{ \
				Sortix::Log::PrintF("Assertion failure: %s:%u : %s %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, #invariant); \
				while ( true ) { } \
			}

		#define LOL(what) #what

		void SysCreateThread(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			Thread* Thread = CreateThread(CurrentProcess(), (Thread::Entry) R->ebx, (void*) R->ecx, (void*) R->edx, (size_t) R->edi);
			R->eax = (Thread != NULL) ? Thread->GetId() : 0;
			//Log::PrintF("<CreatedThread id=\"%p\"/>\n", Thread);
#else
			#warning "This syscall is not supported on this arch"
			while(true);
#endif
		}

		void SysExitThread(CPU::InterruptRegisters* R)
		{
			//Log::PrintF("<ExitedThread id=\"%p\"/>\n", CurrentThread());
#ifdef PLATFORM_X86
			ExitThread(CurrentThread(), (void*) R->ebx);
			Switch(R, 0);
#else
			#warning "This syscall is not supported on this arch"
			while(true);
#endif
			//Log::PrintF("<ExitedThread nextthread=\"%p\"/>\n", CurrentThread());
		}

		void SysSleep(CPU::InterruptRegisters* R)
		{
			//Log::PrintF("<SysSleep>\n");
#ifdef PLATFORM_X86
			intmax_t TimeToSleep = ((uintmax_t) R->ebx) * 1000ULL;
			if ( TimeToSleep == 0 ) { return; }
			CurrentThread()->Sleep(TimeToSleep);
			Switch(R, 0);
			//Log::PrintF("</SysSleep>\n");
#else
			#warning "This syscall is not supported on this arch"
			while(true);
#endif
		}

		void SysUSleep(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			intmax_t TimeToSleep = ((uintmax_t) R->ebx) / 1000ULL;
			if ( TimeToSleep == 0 ) { return; }
			CurrentThread()->Sleep(TimeToSleep);
			Switch(R, 0);
#else
			#warning "This syscall is not supported on this arch"
			while(true);
#endif
		}
	}

	Thread* CurrentThread()
	{
		return Scheduler::currentThread;
	}

	Process* CurrentProcess()
	{
		if ( Scheduler::currentThread != NULL ) { return Scheduler::currentThread->GetProcess(); } else { return NULL; }
	}
}

