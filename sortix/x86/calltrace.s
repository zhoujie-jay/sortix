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

	x86/calltrace.s
	Attempts to unwind the stack and prints the code locations where functions
	were called. This greatly aids debugging.

*******************************************************************************/

.section .text

.global calltrace
.type calltrace, @function
calltrace:
	push %ebp
	movl %esp, %ebp
	xorl %edi, %edi
	movl %ebp, %ebx

calltrace_unwind:
	testl %ebx, %ebx
	jz calltrace_done
	movl 4(%ebx), %esi # Previous EIP
	movl 0(%ebx), %ebx # Previous EBP
	pushl %esi
	pushl %edi
	call calltrace_print_function
	popl %edi
	addl $4, %esp
	incl %edi
	jmp calltrace_unwind

calltrace_done:
	popl %ebp
	retl

