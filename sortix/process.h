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

	process.h
	Describes a process belonging to a subsystem.

******************************************************************************/

#ifndef SORTIX_PROCESS_H
#define SORTIX_PROCESS_H

#include "descriptors.h"

namespace Sortix
{
	class Thread;
	class Process;
	struct ProcessSegment;

	struct ProcessSegment
	{
	public:
		ProcessSegment() : prev(NULL), next(NULL) { }

	public:
		ProcessSegment* prev;
		ProcessSegment* next;
		addr_t position;
		size_t size;

	public:
		 bool Intersects(ProcessSegment* segments);

	};

	class Process
	{
	public:
		Process(addr_t addrspace);
		~Process();

	private:
		addr_t _addrspace;

	public:
		DescriptorTable descriptors;
		ProcessSegment* segments;

	public:
		bool sigint;

	public:
		addr_t _endcodesection; // HACK

	public:
		addr_t GetAddressSpace() { return _addrspace; }
		void ResetAddressSpace();

	public:
		bool IsSane() { return _addrspace != 0; }

	};

	Process* CurrentProcess();

	void SysExecute(CPU::InterruptRegisters* R);
}

#endif

