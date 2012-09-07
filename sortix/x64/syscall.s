/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	x64/syscall.s
	An assembly stub that acts as glue for system calls.

*******************************************************************************/

.global syscall_handler

.section .text
.type syscall_handler, @function
syscall_handler:
	# The processor disabled interrupts during the int $0x80 instruction,
	# however Sortix system calls runs with interrupts enabled such that they
	# can be pre-empted.
	sti

	movl $0, global_errno # Reset errno
	pushq %rbp

	# Grant ourselves kernel permissions to the data segment.
	movl %ds, %ebp
	pushq %rbp
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Make sure the requested system call is valid, if not, then fix it.
	cmp SYSCALL_MAX, %rax
	jae fix_syscall

valid_syscall:
	# Read a system call function pointer.
	xorq %rbp, %rbp
	movq syscall_list(%rbp,%rax,8), %rax

	# Oh how nice, user-space put the parameters in: rdi, rsi, rdx, rcx, r8, r9

	# Call the system call.
	callq *%rax

	# Restore the previous permissions to data segment.
	popq %rbp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Return to user-space, system call result in %rax, errno in %edx.
	popq %rbp
	movl global_errno, %edx

	# If any signals are pending, fire them now.
	movq asm_signal_is_pending, %rdi
	testq %rdi, %rdi
	jnz call_signal_dispatcher

	iretq

fix_syscall:
	# Call the null system call instead.
	xorq %rax, %rax
	jmp valid_syscall

call_signal_dispatcher:
	# We can't return to this location after the signal, since if any system
	# call is made this stack will get reused and all our nice temporaries wil
	# be garbage. We therefore pass the kernel the state to return to and it'll
	# handle it for us when the signal is over.
	movq 0(%rsp), %rdi # userspace rip
	movq 16(%rsp), %rsi # userspace rflags
	movq 24(%rsp), %rcx # userspace rsp, note %rdx is used for errno
	int $130 # Deliver pending signals.
	# If we end up here, it means that the signal didn't override anything and
	# that we should just go ahead and return to userspace ourselves.
	iretq
