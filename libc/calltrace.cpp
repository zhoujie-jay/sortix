/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    calltrace.cpp
    Traverses the stack and prints the callstack.

*******************************************************************************/

#include <calltrace.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

extern "C" void asm_calltrace();

extern "C" void calltrace_print_function(size_t index, unsigned long ip)
{
	fprintf(stdout, "[pid=%i %s] %zu: 0x%lx\n", getpid(),
	        program_invocation_short_name, index, ip);
}

extern "C" void calltrace()
{
	fprintf(stdout, "[pid=%i %s] Calltrace: (%s)\n", getpid(),
	        program_invocation_short_name, program_invocation_name);
	asm_calltrace();
}
