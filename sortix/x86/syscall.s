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
	pushl $0x0
	pushl $0x80

	# Push eax, ecx, edx, ebx, esp, ebp, esi, edi
	pushal

	# Push the user-space data segment.
	movl %ds, %ebp
	pushl %ebp

	# Load the kernel data segment.
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Compabillity with InterruptRegisters.
	movl %cr2, %ebp
	pushl %ebp

	# Store the state structure's pointer so the call can modify it if needed.
	mov %esp, syscall_state_ptr

	# By default, assume the system call was complete.
	movl $0, system_was_incomplete

	# Reset the kernel errno.
	movl $0, errno

	# Make sure the requested system call is valid.
	cmp SYSCALL_MAX, %eax
	jl valid_eax
	xorl %eax, %eax

valid_eax:
	# Read a system call function pointer.
	xorl %ebp, %ebp
	movl syscall_list(%ebp,%eax,4), %eax

	# Give the system call function the values given by user-space.
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx

	# Call the system call.
	calll *%eax

	# Clean up after the call.
	addl $20, %esp

	# Test if the system call was incomplete
	movl system_was_incomplete, %ebx
	testl %ebx, %ebx

	# If the system call was incomplete, the value in %eax is meaningless.
	jg return_to_userspace

	# The system call was completed, so store the return value.
	movl %eax, 36(%esp)

	# Don't forget to update userspace's errno value.
	call update_userspace_errno

return_to_userspace:
	# Compabillity with InterruptRegisters.
	addl $4, %esp

	# Restore the user-space data segment.
	popl %ebp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	popal

	# Compabillity with InterruptRegisters.
	addl $8, %esp

	# Return to user-space.
	iretl

.type resume_syscall, @function
resume_syscall:
	pushl %ebp
	movl %esp, %ebp

	movl 8(%esp), %eax
	movl 16(%esp), %ecx

	pushl 28(%ecx)
	pushl 24(%ecx)
	pushl 20(%ecx)
	pushl 16(%ecx)
	pushl 12(%ecx)
	pushl 8(%ecx)
	pushl 4(%ecx)
	pushl 0(%ecx)

	call *%eax

	addl $32, %esp
	
	leavel
	retl

