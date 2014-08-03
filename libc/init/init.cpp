/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    init/init.cpp
    Initializes the process by setting up the heap, signal handling, static
    memory and other useful things.

*******************************************************************************/

#include <malloc.h>
#include <pthread.h>
#include <string.h>

extern "C" { char* program_invocation_name; }
extern "C" { char* program_invocation_short_name; }

static char* find_last_elem(char* str)
{
	size_t len = strlen(str);
	for ( size_t i = len; i; i-- )
		if ( str[i-1] == '/' )
			return str + i;
	return str;
}

extern "C" void initialize_standard_library(int argc, char* argv[])
{
	const char* argv0 = argc ? argv[0] : "";
	program_invocation_name = (char*) argv0;
	program_invocation_short_name = find_last_elem((char*) argv0);

	// Initialize pthreads.
	pthread_initialize();
}
