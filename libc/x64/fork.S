/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    x64/fork.S
    Assembly functions related to forking x64 processes.

*******************************************************************************/

#define MSRID_FSBASE 0xC0000100
#define MSRID_GSBASE 0xC0000101

.section .text

.globl __sfork
.type __sfork, @function
__sfork:
	pushq %rbp
	movq %rsp, %rbp

	pushq %rdi
	pushq %rsi

	movq $.Lafter_fork, (0 * 8)(%rsi) # rip
	movq $0,   (1 * 8)(%rsi) # rax, result is 0 for child
	movq %rbx, (2 * 8)(%rsi)
	movq %rcx, (3 * 8)(%rsi)
	movq %rdx, (4 * 8)(%rsi)
	movq %rdi, (5 * 8)(%rsi)
	movq %rsi, (6 * 8)(%rsi)
	movq %rsp, (7 * 8)(%rsi)
	movq %rbp, (8 * 8)(%rsi)
	movq %r8,  (9 * 8)(%rsi)
	movq %r9,  (10 * 8)(%rsi)
	movq %r10, (11 * 8)(%rsi)
	movq %r11, (12 * 8)(%rsi)
	movq %r12, (13 * 8)(%rsi)
	movq %r13, (14 * 8)(%rsi)
	movq %r14, (15 * 8)(%rsi)
	movq %r15, (16 * 8)(%rsi)
	pushfq
	popq %rax
	movq %rax, (17 * 8)(%rsi) # rflags
	movl $MSRID_FSBASE, %edi
	call rdmsr
	movq 0(%rsp), %rsi
	movq %rax, (18 * 8)(%rsi) # fsbase
	movl $MSRID_GSBASE, %edi
	call rdmsr
	movq 0(%rsp), %rsi
	movq %rax, (19 * 8)(%rsi) # gsbase
	movq 8(%rsp), %rdi

	call tfork

.Lafter_fork:
	# The value in %rax determines whether we are child or parent. There is no
	# need to clean up the stack from the above pushes, leaveq sets %rsp to %rbp
	# which does that for us.
	leaveq
	retq
.size __sfork, . - __sfork
