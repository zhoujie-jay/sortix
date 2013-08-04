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

    x86/fork.S
    Assembly functions related to forking x86 processes.

*******************************************************************************/

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
