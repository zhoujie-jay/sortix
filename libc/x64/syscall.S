/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * x64/syscall.S
 * Function for performing system calls.
 */

# x86_64 system call conventions:
# interrupt 128
# system call number: %rax
# parameters: %rdi, %rsi, %rdx, %rcx, %r8, %r9
# return value: %rax, %rdx
# return errno: %rcx
# clobbered: %rdi, %rsi, %r8, %r9, %r10, %r11
# preserved: %rbx, %rsp, %rbp, %r12, %r13, %r14, %r15

.global asm_syscall
asm_syscall: /* syscall num in %rax. */
	push %rbp
	mov %rsp, %rbp
	int $0x80
	test %ecx, %ecx
	jz 1f
	mov %fs:0, %rsi
	mov %ecx, errno@tpoff(%rsi)
1:
	pop %rbp
	ret
.size asm_syscall, .-asm_syscall
