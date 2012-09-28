/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011.

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

	start.s
	A stub for linking to the C runtime on Sortix.

*******************************************************************************/

/* NOTE: This is almost identical to what is in crt1.s. crt1.s is used when
         compiling with the real cross compiler, while start.s is used when
         using the older hacky way of building Sortix. Please try to keep these
         files alike by doing changes both places until we only build Sortix
         with real cross compilers. */

.globl _start

.section .text

.type _start, @function
_start:

	movq %rcx, environ # envp

	pushq %rsi
	pushq %rdi

	call initialize_standard_library

	popq %rdi
	popq %rsi

	# Run main
	call main

	# Terminate the process with main's exit code.
	movl %eax, %edi
	call exit

