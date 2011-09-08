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

	process.cpp
	Describes a process belonging to a subsystem.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "process.h"
#include "memorymanagement.h"
#include "initrd.h"
#include "elf.h"

namespace Sortix
{
	bool ProcessSegment::Intersects(ProcessSegment* segments)
	{
		for ( ProcessSegment* tmp = segments; tmp != NULL; tmp = tmp->next )
		{
			if ( tmp->position < position + size &&
			     position < tmp->position + tmp->size )
			{
				return true;
			}
		}

		if ( next ) { return next->Intersects(segments); }

		return false;
	}

	Process::Process(addr_t addrspace)
	{
		_addrspace = addrspace;
		_endcodesection = 0x400000UL;
		segments = NULL;
		sigint = false;
	}

	Process::~Process()
	{
		ResetAddressSpace();

		// Avoid memory leaks.
		ASSERT(segments == NULL);

		// TODO: Delete address space!
	}

	void Process::ResetAddressSpace()
	{
		ProcessSegment* tmp = segments;
		while ( tmp != NULL )
		{
			VirtualMemory::UnmapRangeUser(tmp->position, tmp->size);
			ProcessSegment* todelete = tmp;
			tmp = tmp->next;
			delete todelete;
		}

		segments = NULL;
	}

	void SysExecute(CPU::InterruptRegisters* R)
	{
#ifdef PLATFORM_X86
		const char* programname = (const char*) R->ebx;

		size_t programsize = 0;
		byte* program = InitRD::Open(programname, &programsize);
		if ( program == NULL ) { R->eax = -1; return; }
		addr_t entry = ELF::Construct(CurrentProcess(), program, programsize);
		if ( entry == 0 )
		{
			PanicF("Could not create process '%s'", programname);
		}
		
		// This is a hacky way to set up the thread!
		R->eip = entry;
		R->useresp = 0x80000000UL;
		R->ebp = 0x80000000UL;
#endif
	}
}
