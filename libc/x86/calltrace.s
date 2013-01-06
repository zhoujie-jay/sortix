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

    x86/calltrace.s
    Attempts to unwind the stack and prints the code locations where functions
    were called. This greatly aids debugging.

*******************************************************************************/

.global asm_calltrace
.type asm_calltrace, @function
asm_calltrace:
	push %ebp
	mov %esp, %ebp
	push %ebx
	push %esi
	xor %esi, %esi
	mov %ebp, %ebx

.Lasm_calltrace_unwind:
	test %ebx, %ebx
	jz .Lasm_calltrace_done
	mov 4(%ebx), %ecx # Previous EIP
	mov 0(%ebx), %ebx # Previous EBP
	test %ecx, %ecx
	jz .Lasm_calltrace_done
	push %ecx
	push %esi
	call calltrace_print_function
	add $8, %esp
	inc %esi
	jmp .Lasm_calltrace_unwind

.Lasm_calltrace_done:
	pop %esi
	pop %ebx
	pop %ebp
	ret
.size asm_calltrace, . - asm_calltrace
