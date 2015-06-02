/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

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

    stdlib/mkdtemp.cpp
    Make a unique temporary directory path and create it.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

const uint32_t NUM_CHARACTERS = 10 + 26 + 26;

static inline char random_character()
{
	uint32_t index = arc4random_uniform(NUM_CHARACTERS);
	if ( index < 10 )
		return '0' + index;
	if ( index < 10 + 26 )
		return 'a' + index - 10;
	if ( index < 10 + 26 + 26 )
		return 'A' + index - (10 + 26);
	__builtin_unreachable();
}

extern "C" char* mkdtemp(char* templ)
{
	size_t templ_length = strlen(templ);
	if ( templ_length < 6 )
		return errno = EINVAL, (char*) NULL;
	size_t xpos = templ_length - 6;
	for ( size_t i = 0; i < 6; i++ )
		if ( templ[xpos + i] != 'X' )
			return errno = EINVAL, (char*) NULL;

	do
	{
		for ( size_t i = 0; i < 6; i++ )
			templ[xpos + i] = random_character();
		if ( mkdir(templ, 0700) == 0 )
			return templ;
	} while ( errno == EEXIST );

	memcpy(templ + xpos, "XXXXXX", 6);

	return NULL;
}
