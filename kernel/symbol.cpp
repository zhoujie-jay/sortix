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

    symbol.cpp
    Symbol table access.

*******************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/symbol.h>

namespace Sortix {

Symbol* kernel_symbol_table;
size_t kernel_symbol_table_length;

void SetKernelSymbolTable(Symbol* table, size_t length)
{
	kernel_symbol_table = table;
	kernel_symbol_table_length = length;
}

const Symbol* GetKernelSymbolTable(size_t* length)
{
	if ( length )
		*length = kernel_symbol_table_length;
	return kernel_symbol_table;
}

static bool MatchesSymbol(const Symbol* symbol, uintptr_t address)
{
	return symbol->address <= address &&
	       address <= symbol->address + symbol->size;
}

const Symbol* GetKernelSymbol(uintptr_t address)
{
	for ( size_t i = 0; i < kernel_symbol_table_length; i++ )
	{
		const Symbol* symbol = kernel_symbol_table + i;
		if ( MatchesSymbol(symbol, address) )
			return symbol;
	}
	return NULL;
}

} // namespace Sortix
