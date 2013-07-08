/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	x64/calltrace.s
	Attempts to unwind the stack and prints the code locations where functions
	were called. This greatly aids debugging.

*******************************************************************************/

.section .text

.global calltrace
.type calltrace, @function
calltrace:
	push %rbp
	push %rbx
	movq %rsp, %rbp
	xorl %edi, %edi
	movq %rbp, %rbx

calltrace_unwind:
	testq %rbx, %rbx
	jz calltrace_done
	movq 8(%rbx), %rsi # Previous RIP
	movq 0(%rbx), %rbx # Previous RBP
	pushq %rdi
	call calltrace_print_function
	popq %rdi
	incq %rdi
	jmp calltrace_unwind

calltrace_done:
	popq %rbx
	popq %rbp
	retq

