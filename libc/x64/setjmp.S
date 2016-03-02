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
 * x64/setjmp.S
 * Implement the assembly part of setjmp.
 */

#define SIG_SETMASK 2

/*
 * sigjmp_buf[0] = %rbx
 * sigjmp_buf[1] = %rsp
 * sigjmp_buf[2] = %rbp
 * sigjmp_buf[3] = %r12
 * sigjmp_buf[4] = %r13
 * sigjmp_buf[5] = %r14
 * sigjmp_buf[6] = %r15
 * sigjmp_buf[7] = <return-address>
 * sigjmp_buf[8] = <non-zero if sigmask saved>
 * sigjmp_buf[9...] = <saved sigmask if sigjmp_buf[8] != 0>
 */

.global setjmp
.type setjmp, @function
setjmp:
	movl $1, %esi # setjmp saves the signal mask on Sortix
    jmp 1f
.size setjmp, . - setjmp

.global sigsetjmp
.type sigsetjmp, @function
sigsetjmp:
	mov %esi, %esi # clear upper 32-bit bits
	testl %esi, %esi
	jz 2f
1:	push %rdi
	push %rsi
	lea (9 * 8)(%rdi), %rdx # oldset
	xor %esi, %esi # set
	xor %edi, %edi # how (ignored because set is NULL)
	call sigprocmask # assumes sigprocmask is per-thread on Sortix
	pop %rsi
	pop %rdi
2:	mov 0(%rsp), %rax
	mov %rbx, (0 * 8)(%rdi)
	mov %rsp, (1 * 8)(%rdi)
	mov %rbp, (2 * 8)(%rdi)
	mov %r12, (3 * 8)(%rdi)
	mov %r13, (4 * 8)(%rdi)
	mov %r14, (5 * 8)(%rdi)
	mov %r15, (6 * 8)(%rdi)
	mov %rax, (7 * 8)(%rdi)
	mov %rsi, (8 * 8)(%rdi)
	xorl %eax, %eax
	ret
.size sigsetjmp, . - sigsetjmp

.global longjmp
.type longjmp, @function
longjmp:
    jmp siglongjmp
.size longjmp, . - longjmp

.global siglongjmp
.type siglongjmp, @function
siglongjmp:
	testl %esi, %esi
	jnz 1f
	movl $1, %esi
1:	movq (8 * 8)(%rdi), %rdx
	testq %rdx, %rdx
	jz 2f
	pushq %rdi
	pushq %rsi
	leaq (9 * 8)(%rdi), %rsi # set
	movl $SIG_SETMASK, %edi # how
	xorl %edx, %edx # oldset
	call sigprocmask
	popq %rsi
	popq %rdi
2:	movq (0 * 8)(%rdi), %rbx
	movq (1 * 8)(%rdi), %rsp
	movq (2 * 8)(%rdi), %rbp
	movq (3 * 8)(%rdi), %r12
	movq (4 * 8)(%rdi), %r13
	movq (5 * 8)(%rdi), %r14
	movq (6 * 8)(%rdi), %r15
	movq (7 * 8)(%rdi), %rax
	movq %rax, 0(%rsp)
	movl %esi, %eax
	ret
.size siglongjmp, . - siglongjmp
