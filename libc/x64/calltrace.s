/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    x64/calltrace.s
    Attempts to unwind the stack and prints the code locations where functions
    were called. This greatly aids debugging.

*******************************************************************************/

.global asm_calltrace
.type asm_calltrace, @function
asm_calltrace:
	push %rbp
	mov %rsp, %rbp
	push %rbx
	push %r12
	xor %r12, %r12
	mov %rbp, %rbx

.Lasm_calltrace_unwind:
	test %rbx, %rbx
	jz .Lasm_calltrace_done
	mov 8(%rbx), %rsi # Previous RIP
	mov 0(%rbx), %rbx # Previous RBP
	test %rsi, %rsi
	jz .Lasm_calltrace_done
	mov %r12, %rdi
	call calltrace_print_function
	inc %r12
	jmp .Lasm_calltrace_unwind

.Lasm_calltrace_done:
	pop %r12
	pop %rbx
	pop %rbp
	ret
.size asm_calltrace, . - asm_calltrace
