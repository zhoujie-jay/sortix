/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    x86/crt0.S
    Implement the _start symbol at which execution begins which performs the
    task of initializing the standard library and executing the main function.

*******************************************************************************/

.section .text

.global _start
.global __start
.type _start, @function
.type __start, @function
_start:
__start:
	# Set up end of the stack frame linked list.
	xorl %ebp, %ebp
	pushl %ebp # rip=0
	pushl %ebp # rbp=0
	movl %esp, %ebp

	movl %ecx, environ # envp

	# Arguments for main
	subl $12, %esp
	push %ecx # envp (to main)
	push %ebx # argv (to main)
	push %eax # argc (to main)
	subl $4, %esp
	push %ecx # envp (to initialize_standard_library)
	push %ebx # argv (to initialize_standard_library)
	push %eax # argc (to initialize_standard_library)

	# Initialize the standard library.
	call initialize_standard_library
	addl $16, %esp

	# Run the global constructors.
	call _init

	# Run main
	call main

	# Terminate the process with main's exit code.
	subl $12, %esp
	push %eax
	call exit
.size _start, .-_start
.size __start, .-__start
