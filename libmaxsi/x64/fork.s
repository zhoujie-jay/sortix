/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	x64/fork.s
	Assembly functions related to forking x64 processes.

*******************************************************************************/

.section .text

.globl __call_tfork_with_regs
.type __call_tfork_with_regs, @function
__call_tfork_with_regs:
	pushq %rbp
	movq %rsp, %rbp

	# The actual system call expects a struct tforkregs_x64 containing the state
	# of each register in the child. Since we create an identical copy, we
	# simply set each member of the structure to our own state. Note that since
	# the stack goes downwards, we create it in the reverse order.
	pushfq
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rbp
	pushq %rsp
	pushq %rsi
	pushq %rdi
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq $0 # rax, result of sfork is 0 for the child.
	pushq $after_fork # rip, child will start execution from here.

	# Call tfork with a nice pointer to our structure. Note that %rdi contains
	# the flag parameter that this function accepted.
	movq %rsp, %rsi
	call tfork

after_fork:
	# The value in %rax determines whether we are child or parent. There is no
	# need to clean up the stack from the above pushes, leaveq sets %rsp to %rbp
	# which does that for us.
	leaveq
	retq

