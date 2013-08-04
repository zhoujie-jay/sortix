/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    x86-family/float.cpp
    Handles saving and restoration of floating point numbers.

*******************************************************************************/

#include <assert.h>

#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/thread.h>

#include "float.h"

namespace Sortix {
namespace Float {

static Thread* fputhread;
bool fpu_is_enabled = false;

static inline void InitFPU()
{
	asm volatile ("fninit");
}

static inline void SaveState(uint8_t* dest)
{
	assert( (((unsigned long) dest) & (16UL-1UL)) == 0 );
	asm volatile ("fxsave (%0)" : : "r"(dest));
}

static inline void LoadState(const uint8_t* src)
{
	assert( (((unsigned long) src) & (16UL-1UL)) == 0 );
	asm volatile ("fxrstor (%0)" : : "r"(src));
}

void Yield()
{
	Thread* thread = CurrentThread();

	Interrupt::Disable();

	bool fpu_was_enabled = fpu_is_enabled;

	// The FPU contains the registers for this thread.
	if ( fputhread == thread )
	{
		if ( !fpu_was_enabled )
			EnableFPU();
		SaveState(thread->fpuenvaligned);
		fputhread = NULL;
		DisableFPU();
	}

	// This thread has used the FPU once.
	else if ( thread->fpuinitialized )
	{
		// Nothing needs to be done, the FPU is owned by another thread and the
		// FPU registers are already stored in the thread structure.
	}

	// This thread has never used the FPU and needs its registers initialized.
	else
	{
		if ( !fpu_was_enabled )
			EnableFPU();

		if ( fputhread )
			SaveState(fputhread->fpuenvaligned);

		InitFPU();
		SaveState(thread->fpuenvaligned);

		if ( fputhread )
			LoadState(fputhread->fpuenvaligned);

		if ( !fpu_was_enabled )
			DisableFPU();
	}

	Interrupt::Enable();
}

static void OnFPUAccess(CPU::InterruptRegisters* /*regs*/, void* /*user*/)
{
	EnableFPU();
	Thread* thread = CurrentThread();
	if ( thread == fputhread )
		return;
	if ( fputhread )
		SaveState(fputhread->fpuenvaligned);
	fputhread = thread;
	if ( !thread->fpuinitialized )
	{
		InitFPU();
		thread->fpuinitialized = true;
		return;
	}
	LoadState(thread->fpuenvaligned);
}

void Init()
{
	fputhread = CurrentThread();
	assert(fputhread);
	Interrupt::RegisterHandler(7, OnFPUAccess, NULL);
}

void NofityTaskExit(Thread* thread)
{
	if ( fputhread == thread )
		fputhread = NULL;
	DisableFPU();
}

} // namespace Float
} // namespace Sortix
