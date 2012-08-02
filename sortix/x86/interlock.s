/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	x86/interlock.s
	Functions that perform non-atomic operations in an atomic manner.

*******************************************************************************/

.section .text

# Atomicly runs a function on a value and returns the unmodified value.
# ilret_t
# asm_interlocked_modify(unsigned long* val,
#                        unsigned long (*func)(unsigned long, unsigned long),
#                        unsigned long user);
.global asm_interlocked_modify
.type asm_interlocked_modify, @function
asm_interlocked_modify:
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx
asm_interlocked_modify_retry:
	# Read the current value and calculate the replacement.
	movl 20(%ebp), %ecx # user
	movl 16(%ebp), %edx # func
	movl 12(%ebp), %ebx # val
	movl (%ebx), %ebx # *val
	pushl %ecx
	pushl %ebx
	call *%edx
	# Atomicly assign the replacement if the value is unchanged.
	movl %eax, %edx # New value in %edx
	movl %ebx, %eax # Old value in %eax
	movl 8(%ebp), %ebx # val
	cmpxchgl %edx, (%ebx)
	# Retry if the value was modified by someone else.
	jnz asm_interlocked_modify_retry
	# According to the calling convention, the first parameter is secretly a
	# pointer to where we store the result, the old value first, then the new.
	movl 8(%ebp), %ebx
	movl %eax, 0(%ebx)
	movl %edx, 4(%ebx)
	popl %ebx
	leavel
	retl $0x4
