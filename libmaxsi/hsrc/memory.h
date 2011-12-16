/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	memory.h
	Useful functions for manipulating memory, as well as the implementation
	of the heap.

******************************************************************************/

#ifndef LIBMAXSI_MEMORY_H
#define LIBMAXSI_MEMORY_H

namespace Maxsi
{
	namespace Memory
	{
		void	Init();
		void*	Allocate(size_t Size);
		void	Free(void* Addr);
		size_t	GetChunkOverhead();
		void*	Copy(void* Dest, const void* Src, size_t Length);
		void*	Set(void* Dest, int Value, size_t Length);
	}
}

// Placement new.
inline void* operator new(size_t, void* p)     { return p; }
inline void* operator new[](size_t, void* p)   { return p; }
inline void  operator delete  (void*, void*) { };
inline void  operator delete[](void*, void*) { };

#endif

