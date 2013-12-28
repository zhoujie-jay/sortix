/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    x86/setjmp.S
    Implement the assembly part of setjmp.

*******************************************************************************/

.global setjmp
.type setjmp, @function
setjmp:
	mov 4(%esp), %ecx
	# TODO: Floating point stuff!
	mov %ebx, 0x00(%ecx)
	mov %esi, 0x04(%ecx)
	mov %edi, 0x08(%ecx)
	mov %ebp, 0x0C(%ecx)
	mov %esp, 0x10(%ecx)
	mov 0(%esp), %eax
	mov %eax, 0x14(%ecx)
	xorl %eax, %eax
.Lsetjmp_return:
	ret
.size setjmp, . - setjmp

.global longjmp
.type longjmp, @function
longjmp:
	mov 4(%esp), %ecx
	mov 8(%esp), %edx
	testl %edx, %edx
	jnz 1f
	mov $1, %edx
1:
	# TODO: Floating point stuff!
	mov 0x04(%ecx), %esi
	mov 0x08(%ecx), %edi
	mov 0x0C(%ecx), %ebp
	mov 0x10(%ecx), %esp
	mov 0x14(%ecx), %eax
	mov %eax, 0(%esp)
	mov %edx, %eax
	jmp .Lsetjmp_return
.size longjmp, . - longjmp
