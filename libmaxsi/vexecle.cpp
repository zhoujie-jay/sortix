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

	vexecle.cpp
	Loads a process image.

*******************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" int vexecle(const char* pathname, va_list args)
{
	va_list iter;
	va_copy(iter, args);
	size_t numargs = 0;
	while ( va_arg(iter, const char*) ) { numargs++; }
	va_end(iter);
	numargs--; // envp
	char** argv = (char**) malloc(sizeof(char*) * (numargs+1));
	if ( !argv ) { return -1; }
	for ( size_t i = 0; i < numargs; i++ )
	{
		argv[i] = (char*) va_arg(args, const char*);
	}
	argv[numargs] = NULL;
	char* const* envp = va_arg(args, char* const*);
	int result = execve(pathname, argv, envp);
	free(argv);
	return result;
}
