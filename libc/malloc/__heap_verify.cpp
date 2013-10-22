/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    malloc/__heap_verify.cpp
    Perform a heap consistency check.

*******************************************************************************/

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>

extern "C" void __heap_verify()
{
	for ( size_t i = 0; i < sizeof(size_t) * 8 - 1; i++ )
	{
		if ( __heap_state.bin_filled_bitmap & heap_size_of_bin(i) )
		{
			assert(__heap_state.bin[i] != NULL);
		}
		else
		{
			assert(__heap_state.bin[i] == NULL);
		}
	}
}
