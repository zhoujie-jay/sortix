/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	start.s
	A stub for linking to the C runtime on Sortix. 

******************************************************************************/

.globl _start

.section .text

.type _start, @function
_start:

	call initialize_standard_library

	# Run main
	# TODO: Sortix should set the initial values before this point!
	call main

	# Terminate the process with main's exit code.
	movl %eax, %edi
	call exit

