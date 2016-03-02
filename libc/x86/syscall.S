/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * x86/syscall.S
 * Function for performing system calls.
 */

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
	mov %gs:0, %ebx
	mov %ecx, errno@ntpoff(%ebx)
1:
	pop %esi
	pop %edi
	pop %ebx
	pop %ebp
	ret
.size asm_syscall, .-asm_syscall
