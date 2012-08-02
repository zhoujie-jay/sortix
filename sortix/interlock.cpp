/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	interlock.cpp
	Functions that perform non-atomic operations in an atomic manner.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/interlock.h>

namespace Sortix {

// TODO: This is likely not the most optimal way to perform these operations.

extern "C" ilret_t asm_interlocked_modify(unsigned long* val,
                                          ilockfunc f,
                                           unsigned long user);

ilret_t InterlockedModify(unsigned long* ptr,
                          ilockfunc f,
                          unsigned long user)
{
	return asm_interlocked_modify(ptr, f, user);
}

static unsigned long AddFunction(unsigned long val, unsigned long arg)
{
	return val + arg;
}

static unsigned long SubFunction(unsigned long val, unsigned long arg)
{
	return val - arg;
}

ilret_t InterlockedIncrement(unsigned long* ptr)
{
	return InterlockedModify(ptr, AddFunction, 1);
}

ilret_t InterlockedDecrement(unsigned long* ptr)
{
	return InterlockedModify(ptr, SubFunction, 1);
}

ilret_t InterlockedAdd(unsigned long* ptr, unsigned long arg)
{
	return InterlockedModify(ptr, AddFunction, arg);
}

ilret_t InterlockedSub(unsigned long* ptr, unsigned long arg)
{
	return InterlockedModify(ptr, SubFunction, arg);
}

} // namespace Sortix
