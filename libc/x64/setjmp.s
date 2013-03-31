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

    x64/setjmp.s
    Implement the assembly part of setjmp.

*******************************************************************************/

.global setjmp
.type setjmp, @function
setjmp:
	# TODO: Floating point stuff!
	mov %rbx, 0x00(%rdi)
	mov %rsp, 0x08(%rdi)
	mov %rbp, 0x10(%rdi)
	mov %r12, 0x18(%rdi)
	mov %r13, 0x20(%rdi)
	mov %r14, 0x28(%rdi)
	mov %r15, 0x30(%rdi)
	mov 0(%rsp), %rax
	mov %rax, 0x38(%rdi)
	xorl %eax, %eax
.Lsetjmp_return:
	ret
.size setjmp, . - setjmp

.global longjmp
.type longjmp, @function
longjmp:
	testl %esi, %esi
	jnz 1f
	mov $1, %esi
1:
	# TODO: Floating point stuff!
	mov 0x00(%rdi), %rbx
	mov 0x08(%rdi), %rsp
	mov 0x10(%rdi), %rbp
	mov 0x18(%rdi), %r12
	mov 0x20(%rdi), %r13
	mov 0x28(%rdi), %r14
	mov 0x30(%rdi), %r15
	mov 0x38(%rdi), %rax
	mov %rax, 0(%rsp)
	mov %esi, %eax
	jmp .Lsetjmp_return
.size longjmp, . - longjmp
