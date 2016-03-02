/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * An assembly stub that acts as glue for system calls.
 */

#include <sortix/syscall.h>

.global syscall_handler

.section .text
.type syscall_handler, @function
syscall_handler:
	/* -- stack is 12 bytes from being 16-byte aligned -- */
	movl $0, global_errno # Reset errno

	pushl %ebp
	/* -- stack is 8 bytes from being 16-byte aligned -- */

	# Grant ourselves kernel permissions to the data segment.
	movl %ds, %ebp
	pushl %ebp
	/* -- stack is 4 bytes from being 16-byte aligned -- */
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es

	# Make sure the requested system call is valid.
	cmp $SYSCALL_MAX_NUM, %eax
	jae 3f

1:
	# Read a system call function pointer.
	xorl %ebp, %ebp
	movl syscall_list(%ebp,%eax,4), %eax

	# Point the %ebp register to the pushed user-space %ebp above.
	movl %esp, %ebp
	addl $4, %ebp

	# Call the system call.
	pushl %esi
	/* -- stack is 16-byte aligned -- */
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	/* -- stack is 16-byte aligned -- */
	calll *%eax
	addl $20, %esp

	# Restore the previous permissions to data segment.
	popl %ebp
	movl %ebp, %ds
	movl %ebp, %es

	# Return to user-space, system call result in %eax:%edx, errno in %ecx.
	popl %ebp
	movl global_errno, %ecx

	# Zero registers to avoid information leaks.
	# eax is the return value.
	# ecx is errno.
	# edx is the return value (MIGHT NOT BE INITIALIZED, CAN LEAK!).
	# ebx is set in a moment.
	xor %esi, %esi
	xor %edi, %edi
	# If any signals are pending, fire them now.
	movl asm_signal_is_pending, %ebx
	testl %ebx, %ebx
	jnz 4f
	# ebx is zero in this branch.

2:
	iretl

3:
	# Call the null system call instead.
	xorl %eax, %eax
	jmp 1b

4:
	# We can't return to this location after the signal, since if any system
	# call is made this stack will get reused and all our nice temporaries wil
	# be garbage. We therefore pass the kernel the state to return to and it'll
	# handle it for us when the signal is over.
	movl %esp, %ebx
	int $130 # Deliver pending signals.
	# If we end up here, it means that the signal didn't override anything and
	# that we should just go ahead and return to userspace ourselves.
	xor %ebx, %ebx
	jmp 2b

.size syscall_handler, .-syscall_handler
