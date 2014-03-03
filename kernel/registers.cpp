/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    registers.cpp
    Register structures and platform-specific bits.

*******************************************************************************/

#include <stdint.h>

#include <sortix/kernel/log.h>
#include <sortix/kernel/registers.h>
#include <sortix/kernel/scheduler.h>

namespace Sortix {

void LogInterruptContext(const struct interrupt_context* intctx)
{
#if defined(__i386__)
	Log::PrintF("[cr2=0x%zx,ds=0x%zx,edi=0x%zx,esi=0x%zx,ebp=0x%zx,"
	            "ebx=0x%zx,edx=0x%zx,ecx=0x%zx,eax=0x%zx,"
	            "int_no=0x%zx,err_code=0x%zx,eip=0x%zx,cs=0x%zx,"
	            "eflags=0x%zx,esp=0x%zx,ss=0x%zx]",
	            intctx->cr2, intctx->ds, intctx->edi, intctx->esi, intctx->ebp,
	            intctx->ebx, intctx->edx, intctx->ecx, intctx->eax,
	            intctx->int_no, intctx->err_code, intctx->eip, intctx->cs,
	            intctx->eflags, intctx->esp, intctx->ss);
#elif defined(__x86_64__)
	Log::PrintF("[cr2=0x%zx,ds=0x%zx,rdi=0x%zx,rsi=0x%zx,rbp=0x%zx,"
	            "rbx=0x%zx,rdx=0x%zx,rcx=0x%zx,rax=0x%zx,"
	            "r8=0x%zx,r9=0x%zx,r10=0x%zx,r11=0x%zx,r12=0x%zx,"
	            "r13=0x%zx,r14=0x%zx,r15=0x%zx,int_no=0x%zx,"
	            "err_code=0x%zx,rip=0x%zx,cs=0x%zx,rflags=0x%zx,"
	            "rsp=0x%zx,ss=0x%zx]",
	            intctx->cr2, intctx->ds, intctx->rdi, intctx->rsi, intctx->rbp,
	            intctx->rbx, intctx->rdx, intctx->rcx, intctx->rax,
	            intctx->r8, intctx->r9, intctx->r10, intctx->r11, intctx->r12,
	            intctx->r13, intctx->r14, intctx->r15, intctx->int_no,
	            intctx->err_code, intctx->rip, intctx->cs, intctx->rflags,
	            intctx->rsp, intctx->ss);
#else
#warning "You need to implement printing an interrupt context"
#endif
}

extern "C" __attribute__((noreturn))
void load_registers(const struct interrupt_context* inctx);

__attribute__((noreturn))
void LoadRegisters(const struct thread_registers* registers)
{
	struct interrupt_context intctx;
	memset(&intctx, 0, sizeof(intctx));

	Scheduler::LoadInterruptedContext(&intctx, registers);

	load_registers(&intctx);
}

} // namespace Sortix
