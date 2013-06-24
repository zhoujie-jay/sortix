/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    stdlib/mktemp.cpp
    Make a unique temporary filename.

*******************************************************************************/

#include <stdlib.h>
#include <string.h>

// TODO: This is a hacky implementation of a stupid function.
extern "C" char* mktemp(char* templ)
{
	size_t templlen = strlen(templ);
	for ( size_t i = templlen-6UL; i < templlen; i++ )
	{
		templ[i] = '0' + rand() % 10;
	}
	return templ;
}
