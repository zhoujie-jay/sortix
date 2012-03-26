/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	syscall.s
	An assembly stub that acts as glue for system calls.

******************************************************************************/

.global syscall_handler
.global resume_syscall

.section .text
.type syscall_handler, @function
syscall_handler:
	cli

	# Compabillity with InterruptRegisters.
	pushq $0x0
	pushq $0x80

	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %rbx
	pushq %rsp
	pushq %rbp
	pushq %rsi
	pushq %rdi

	# Push the user-space data segment.
	movl %ds, %ebp
	pushq %rbp

	# Load the kernel data segment.
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Compabillity with InterruptRegisters.
	movq %cr2, %rbp
	pushq %rbp

	# Store the state structure's pointer so the call can modify it if needed.
	movq %rsp, syscall_state_ptr

	# By default, assume the system call was complete.
	movl $0, system_was_incomplete

	# Reset the kernel errno.
	movl $0, global_errno

	# Make sure the requested system call is valid.
	cmp SYSCALL_MAX, %rax
	jb valid_rax
	xorq %rax, %rax

valid_rax:
	# Read a system call function pointer.
	xorq %rbp, %rbp
	movq syscall_list(%rbp,%rax,8), %rax

	# Oh how nice, user-space put the parameters in: rdi, rsi, rdx, rcx, r8, r9

	# Call the system call.
	callq *%rax

	# Test if the system call was incomplete
	movl system_was_incomplete, %ebx
	testl %ebx, %ebx

	# If the system call was incomplete, the value in %eax is meaningless.
	jg return_to_userspace

	# The system call was completed, so store the return value.
	movq %rax, 72(%rsp)

	# Don't forget to update userspace's errno value.
	call update_userspace_errno

return_to_userspace:
	# Compabillity with InterruptRegisters.
	addq $8, %rsp

	# Restore the user-space data segment.
	popq %rbp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	popq %rdi
	popq %rsi
	popq %rbp
	popq %rsp
	popq %rbx
	popq %rdx
	popq %rcx
	popq %rax
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15

	# Compabillity with InterruptRegisters.
	addq $16, %rsp

	# Return to user-space.
	iretq

.type resume_syscall, @function
resume_syscall:
	pushq %rbp
	movq %rsp, %rbp

	movq %rdi, %rax
	movq %rdx, %r11

	movq 0(%r11), %rdi
	movq 8(%r11), %rsi
	movq 16(%r11), %rdx
	movq 24(%r11), %rcx
	movq 32(%r11), %r8
	movq 40(%r11), %r9

	callq *%rax

	leaveq
	retq

