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

    x86/syscall.s
    Function for performing system calls.

*******************************************************************************/

# i386 system call conventions:
# interrupt 128
# system call number: %eax
# parameters: %ebx, %ecx, %edx, %edi, %esi
# return value: %eax, %edx
# return errno: %ecx
# clobbered: %ebx, %edi, %esi
# preserved: %ebp, %esp

.section .text
.global asm_syscall
asm_syscall: /* syscall num in %eax. */
	push %ebp
	mov %esp, %ebp
	push %ebx
	push %edi
	push %esi
	#   0 (%ebp) # saved_ebp
	#   4 (%ebp) # return_eip
	mov 8 (%ebp), %ebx # param_word1
	mov 12(%ebp), %ecx # param_word2
	mov 16(%ebp), %edx # param_word3
	mov 20(%ebp), %edi # param_word4
	mov 24(%ebp), %esi # param_word5
	int $0x80
	test %ecx, %ecx # ret_errno & ret_errno
	jz 1f # if ( !(ret_errno & ret_errno) )
	mov %eax, %ebx # preserve: ret_low
	mov %edx, %edi # preserve: ret_high
	mov %ecx, %esi # preserve: ret_errno
	call get_errno_location # get_errno_location()
	mov %esi, (%eax) # *get_errno_location() = ret_errno
	mov %ebx, %eax # restore: ret_low
	mov %edi, %edx # restore: ret_high
1:
	pop %esi
	pop %edi
	pop %ebx
	pop %ebp
	ret
.size asm_syscall, .-asm_syscall
