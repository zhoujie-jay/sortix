/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    x64/syscall.S
    Function for performing system calls.

*******************************************************************************/

# x86_64 system call conventions:
# interrupt 128
# system call number: %rax
# parameters: %rdi, %rsi, %rdx, %rcx, %r8, %r9
# return value: %rax, %rdx
# return errno: %rcx
# clobbered: %rdi, %rsi, %r8, %r9, %r10, %r11
# preserved: %rbx, %rsp, %rbp, %r12, %r13, %r14, %r15

.global asm_syscall
asm_syscall: /* syscall num in %rax. */
	push %rbp
	mov %rsp, %rbp
	int $0x80
	test %ecx, %ecx
	jz 1f
	mov %fs:0, %rsi
	mov %ecx, errno@tpoff(%rsi)
1:
	pop %rbp
	ret
.size asm_syscall, .-asm_syscall
