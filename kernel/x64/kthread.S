/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * x64/kthread.S
 * Utilities and synchronization mechanisms for x64 kernel threads.
 */

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
.size kthread_mutex_trylock, . - kthread_mutex_trylock

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
.size kthread_mutex_lock, . - kthread_mutex_lock

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
.size kthread_mutex_lock_signal, . - kthread_mutex_lock_signal

.global kthread_mutex_unlock
.type kthread_mutex_unlock, @function
kthread_mutex_unlock:
	pushq %rbp
	movq %rsp, %rbp
	movl $0, (%rdi)
	leaveq
	retq
.size kthread_mutex_unlock, . - kthread_mutex_unlock
