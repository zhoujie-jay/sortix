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

	x64.cpp
	CPU stuff for the x64 platform.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include "x64.h"
#include <sortix/kernel/log.h>

namespace Sortix
{
	namespace X64
	{
		void InterruptRegisters::LogRegisters() const
		{
			Log::PrintF("[cr2=0x%zx,ds=0x%zx,rdi=0x%zx,rsi=0x%zx,rbp=0x%zx,"
			            "rsp=0x%zx,rbx=0x%zx,rcx=0x%zx,rax=0x%zx,r8=0x%zx,"
			            "r9=0x%zx,r10=0x%zx,r11=0x%zx,r12=0x%zx,r13=0x%zx,"
			            "r14=0x%zx,r15=0x%zx,int_no=0x%zx,err_code=0x%zx,"
			            "rip=0x%zx,cs=0x%zx,rflags=0x%zx,userrsp=0x%zx,"
			            "ss=0x%zx]",
			            cr2, ds, rdi, rsi, rbp,
			            rsp, rbx, rcx, rax, r8,
			            r9, r10, r11, r12, r13,
			            r14, r15, int_no, err_code,
			            rip, cs, rflags, userrsp,
			            ss);
		}
	}
}
