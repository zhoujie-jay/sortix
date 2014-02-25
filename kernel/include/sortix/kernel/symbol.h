/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/symbol.h
    Symbol table declarations.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_SYMBOL_H
#define INCLUDE_SORTIX_KERNEL_SYMBOL_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {

struct Symbol
{
	uintptr_t address;
	size_t size;
	const char* name;
};

void SetKernelSymbolTable(Symbol* table, size_t length);
const Symbol* GetKernelSymbolTable(size_t* length = NULL);
const Symbol* GetKernelSymbol(uintptr_t address);

static inline const char* GetKernelSymbolName(uintptr_t address)
{
	const Symbol* symbol = GetKernelSymbol(address);
	return symbol ? symbol->name : NULL;
}

} // namespace Sortix

#endif
