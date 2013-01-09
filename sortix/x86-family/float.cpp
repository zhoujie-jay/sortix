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

    x86-family/float.cpp
    Handles saving and restoration of floating point numbers.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/interrupt.h>

#include <assert.h>

#include "../thread.h"
#include "float.h"

namespace Sortix {
namespace Float {

static Thread* fputhread;

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
