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

    x64/crt0.S
    Implement the _start symbol at which execution begins which performs the
    task of initializing the standard library and executing the main function.

*******************************************************************************/

.section .text

.global _start
.type _start, @function
_start:
	# Set up end of the stack frame linked list.
	xorl %ebp, %ebp
	pushq %rbp # rip=0
	pushq %rbp # rbp=0
	movq %rsp, %rbp

	movq %rcx, environ # envp

	# Initialize the standard library.
	subq $8, %rsp
	pushq %rsi
	pushq %rdi
	pushq %rcx
	call initialize_standard_library

	# Run the global constructors.
	call _init

	# Run main
	popq %rdx # Note! envp is now %rdx (previously %rcx)
	popq %rdi
	popq %rsi
	addq $8, %rsp
	call main

	# Terminate the process with main's exit code.
	movl %eax, %edi
	call exit
.size _start, .-_start
