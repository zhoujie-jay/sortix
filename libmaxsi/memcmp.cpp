/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	memcmp.cpp
	Compares two memory regions.

*******************************************************************************/

#include <stdint.h>
#include <string.h>

extern "C" int memcmp(const void* a, const void* b, size_t size)
{
	const uint8_t* buf1 = (const uint8_t*) a;
	const uint8_t* buf2 = (const uint8_t*) b;
	for ( size_t i = 0; i < size; i++ )
	{
		if ( buf1[i] != buf2[i] ) { return (int)(buf1[i]) - (int)(buf2[i]); }
	}
	return 0;
}
