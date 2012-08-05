/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	sortix/x86/fork.h
	Declarations related to the fork family of system calls on x86 Sortix.

*******************************************************************************/

#ifndef SORTIX_X86_FORK_H
#define SORTIX_X86_FORK_H

#include <features.h>

__BEGIN_DECLS

struct tforkregs_x86
{
	uint32_t eip;
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t edi;
	uint32_t esi;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eflags;
};

__END_DECLS

#endif

