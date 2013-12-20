/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    x86/x86.cpp
    CPU stuff for the x86 platform.

*******************************************************************************/

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/log.h>

namespace Sortix
{
	namespace X86
	{
		void InterruptRegisters::LogRegisters() const
		{
			Log::PrintF("[cr2=0x%zx,ds=0x%zx,edi=0x%zx,esi=0x%zx,ebp=0x%zx,"
			            "esp=0x%zx,ebx=0x%zx,edx=0x%zx,ecx=0x%zx,eax=0x%zx,"
			            "int_no=0x%zx,err_code=0x%zx,eip=0x%zx,cs=0x%zx,"
			            "eflags=0x%zx,useresp=0x%zx,ss=0x%zx]",
			            cr2, ds, edi, esi, ebp,
			            esp, ebx, edx, ecx, eax,
			            int_no, err_code, eip, cs,
			            eflags, useresp, ss);
		}
	}
}
