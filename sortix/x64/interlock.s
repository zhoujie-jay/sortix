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

	x64/interlock.s
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
	pushq %rbp
	movq %rsp, %rbp
asm_interlocked_modify_retry:
	# Save our parameters in case we need to retry.
	pushq %rdi
	pushq %rsi
	pushq %rdx
	# Read the current value and calculate the replacement.
	movq (%rdi), %rdi
	movq %rdx, %rsi
	callq *%rsi
	movq %rax, %rdx
	popq %rax
	popq %rdi
	popq %rsi
	# Atomicly assign the replacement if the value is unchanged.
	cmpxchgq %rdx, (%rdi)
	# Retry if the value was modified by someone else.
	jnz asm_interlocked_modify_retry
	# Return the old value in %rax, new one in %rdx.
	leaveq
	retq
