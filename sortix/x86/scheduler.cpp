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

	x86/scheduler.cpp
	CPU specific things related to the scheduler.

******************************************************************************/

#include "platform.h"
#include "scheduler.h"
#include "../memorymanagement.h"
#include "descriptor_tables.h"

namespace Sortix
{
	namespace Scheduler
	{
		const size_t KERNEL_STACK_SIZE = 256UL * 1024UL;
		const addr_t KERNEL_STACK_END = 0x80001000UL;
		const addr_t KERNEL_STACK_START = KERNEL_STACK_END + KERNEL_STACK_SIZE;

		void InitCPU()
		{
			// TODO: Prevent the kernel heap and other stuff from accidentally
			// also allocating this virtual memory region!

			if ( !Memory::MapRangeKernel(KERNEL_STACK_END, KERNEL_STACK_SIZE) )
			{
				PanicF("could not create kernel stack (%zx to %zx)",
				       KERNEL_STACK_END, KERNEL_STACK_START);
			}

			GDT::SetKernelStack((size_t*) KERNEL_STACK_START);
		}
	}
}
