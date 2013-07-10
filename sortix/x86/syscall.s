/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    x86/syscall.s
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
	pushl %ebp

	# Grant ourselves kernel permissions to the data segment.
	movl %ds, %ebp
	pushl %ebp
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Make sure the requested system call is valid.
	cmp SYSCALL_MAX, %eax
	jae fix_syscall

valid_syscall:
	# Read a system call function pointer.
	xorl %ebp, %ebp
	movl syscall_list(%ebp,%eax,4), %eax

	# Call the system call.
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	calll *%eax
	addl $20, %esp

	# Restore the previous permissions to data segment.
	popl %ebp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Return to user-space, system call result in %eax:%edx, errno in %ecx.
	popl %ebp
	movl global_errno, %ecx

	# If any signals are pending, fire them now.
	movl asm_signal_is_pending, %ebx
	testl %ebx, %ebx
	jnz call_signal_dispatcher

	iretl

fix_syscall:
	# Call the null system call instead.
	xorl %eax, %eax
	jmp valid_syscall

call_signal_dispatcher:
	# We can't return to this location after the signal, since if any system
	# call is made this stack will get reused and all our nice temporaries wil
	# be garbage. We therefore pass the kernel the state to return to and it'll
	# handle it for us when the signal is over.
	movl %esp, %ebx
	int $130 # Deliver pending signals.
	# If we end up here, it means that the signal didn't override anything and
	# that we should just go ahead and return to userspace ourselves.
	iretl

.size syscall_handler, .-syscall_handler
