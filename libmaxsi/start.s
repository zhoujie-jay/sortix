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

.text  0x100000
.type _start, @function
_start:

	# Run main
	# TODO: Sortix should push these values to the stack itself!
	push $0x0 # argv
	push $0 # argc
	call main

	# Now return mains result when exiting the process
	mov %eax, %ebx
	mov $1, %eax
	mov $0x0, %ebx # TODO: This syscall exits a thread, not a process!
	int $0x80
