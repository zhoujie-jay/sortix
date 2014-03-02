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

    x86/kthread.S
    Utilities and synchronization mechanisms for x86 kernel threads.

*******************************************************************************/

.section .text

.global kthread_mutex_trylock
.type kthread_mutex_trylock, @function
kthread_mutex_trylock:
	pushl %ebp
	movl %esp, %ebp
	movl 8(%ebp), %edx
	movl $-1, %eax
	xchgl (%edx), %eax
	not %eax
	leavel
	retl
.size kthread_mutex_trylock, . - kthread_mutex_trylock

.global kthread_mutex_lock
.type kthread_mutex_lock, @function
kthread_mutex_lock:
	pushl %ebp
	movl %esp, %ebp
	movl 8(%ebp), %edx
kthread_mutex_lock_retry:
	movl $-1, %eax
	xchgl (%edx), %eax
	testl %eax, %eax
	jnz kthread_mutex_lock_failed
	leavel
	retl
kthread_mutex_lock_failed:
	int $0x81 # Yield the CPU.
	jmp kthread_mutex_lock_retry
.size kthread_mutex_lock, . - kthread_mutex_lock

.global kthread_mutex_lock_signal
.type kthread_mutex_lock_signal, @function
kthread_mutex_lock_signal:
	pushl %ebp
	movl %esp, %ebp
	movl 8(%ebp), %edx
kthread_mutex_lock_signal_retry:
	movl asm_signal_is_pending, %eax
	testl %eax, %eax
	jnz kthread_mutex_lock_signal_pending
	movl $-1, %eax
	xchgl (%edx), %eax
	testl %eax, %eax
	jnz kthread_mutex_lock_signal_failed
	inc %eax
kthread_mutex_lock_signal_out:
	leavel
	retl
kthread_mutex_lock_signal_failed:
	int $0x81 # Yield the CPU.
	jmp kthread_mutex_lock_signal_retry
kthread_mutex_lock_signal_pending:
	xorl %eax, %eax
	jmp kthread_mutex_lock_signal_out

.global kthread_mutex_unlock
.type kthread_mutex_unlock, @function
kthread_mutex_unlock:
	pushl %ebp
	movl %esp, %ebp
	movl 8(%ebp), %edx
	movl $0, (%edx)
	leavel
	retl
.size kthread_mutex_lock_signal, . - kthread_mutex_lock_signal
