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

	x64/kthread.s
	Utilities and synchronization mechanisms for x64 kernel threads.

*******************************************************************************/

.section .text

.global kthread_mutex_trylock
.type kthread_mutex_trylock, @function
kthread_mutex_trylock:
	pushq %rbp
	movq %rsp, %rbp
	movl $-1, %eax
	xchgl (%rdi), %eax
	not %eax
	leaveq
	retq

.global kthread_mutex_lock
.type kthread_mutex_lock, @function
kthread_mutex_lock:
	pushq %rbp
	movq %rsp, %rbp
kthread_mutex_lock_retry:
	movl $-1, %eax
	xchgl (%rdi), %eax
	testl %eax, %eax
	jnz kthread_mutex_lock_failed
	leaveq
	retq
kthread_mutex_lock_failed:
	int $0x81 # Yield the CPU.
	jmp kthread_mutex_lock_retry

.global kthread_mutex_lock_signal
.type kthread_mutex_lock_signal, @function
kthread_mutex_lock_signal:
	pushq %rbp
	movq %rsp, %rbp
kthread_mutex_lock_signal_retry:
	movq asm_signal_is_pending, %rax
	testq %rax, %rax
	jnz kthread_mutex_lock_signal_pending
	movl $-1, %eax
	xchgl (%rdi), %eax
	testl %eax, %eax
	jnz kthread_mutex_lock_signal_failed
	inc %eax
kthread_mutex_lock_signal_out:
	leaveq
	retq
kthread_mutex_lock_signal_failed:
	int $0x81 # Yield the CPU.
	jmp kthread_mutex_lock_signal_retry
kthread_mutex_lock_signal_pending:
	xorl %eax, %eax
	jmp kthread_mutex_lock_signal_out

.global kthread_mutex_unlock
.type kthread_mutex_unlock, @function
kthread_mutex_unlock:
	pushq %rbp
	movq %rsp, %rbp
	movl $0, (%rdi)
	leaveq
	retq
