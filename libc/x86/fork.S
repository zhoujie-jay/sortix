/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * x86/fork.S
 * Assembly functions related to forking x86 processes.
 */

#define MSRID_FSBASE 0xC0000100
#define MSRID_GSBASE 0xC0000101

.section .text

.globl __sfork
.type __sfork, @function
__sfork:
	pushl %ebp
	movl %esp, %ebp

	movl 12(%ebp), %ecx
	push %ecx
	movl 8(%ebp), %edx
	push %edx
	# -- stack is 16-byte aligned -- #

	movl $.Lafter_fork, (0 * 4)(%ecx) # eip
	movl $0,   (1 * 4)(%ecx) # rax, result is 0 for child
	movl %ebx, (2 * 4)(%ecx)
	movl %ecx, (3 * 4)(%ecx)
	movl %edx, (4 * 4)(%ecx)
	movl %edi, (5 * 4)(%ecx)
	movl %esi, (6 * 4)(%ecx)
	movl %esp, (7 * 4)(%ecx)
	movl %ebp, (8 * 4)(%ecx)
	pushfl
	popl %eax
	movl %eax, (9 * 4)(%ecx) # eflags

	subl $12, %esp
	pushl $MSRID_FSBASE
	call rdmsr
	movl 12(%ebp), %ecx
	movl %eax, (10 * 4)(%ecx) # fsbase
	addl $16, %esp

	subl $12, %esp
	pushl $MSRID_GSBASE
	call rdmsr
	movl 12(%ebp), %ecx
	movl %eax, (11 * 4)(%ecx) # gsbase
	addl $16, %esp

	call tfork

.Lafter_fork:
	# The value in %eax: determines whether we are child or parent. There is no
	# need to clean up the stack from the above pushes, leavel sets %esp to %ebp
	# which does that for us.
	leavel
	retl
.size __sfork, . - __sfork
