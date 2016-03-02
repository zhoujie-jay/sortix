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
 * x86/setjmp.S
 * Implement the assembly part of setjmp.
 */

#define SIG_SETMASK 2

/*
 * sigjmp_buf[0] = %ebx
 * sigjmp_buf[1] = %esi
 * sigjmp_buf[2] = %edi
 * sigjmp_buf[3] = %ebp
 * sigjmp_buf[4] = %esp
 * sigjmp_buf[5] = <return-address>
 * sigjmp_buf[6] = <non-zero if sigmask saved>
 * sigjmp_buf[7...] = <saved sigmask if sigjmp_buf[6] != 0>
 */

.global setjmp
.type setjmp, @function
setjmp:
	# setjmp saves the signal mask on Sortix
	jmp 1f
.size setjmp, . - setjmp

.global sigsetjmp
.type sigsetjmp, @function
sigsetjmp:
	movl 8(%esp), %edx
	testl %edx, %edx
	jz 2f
1:	movl 4(%esp), %ecx
	leal (7 * 4)(%ecx), %edx
	pushl %edx # oldset
	pushl $0 # set
	pushl $0 # how (ignored because set is NULL)
	call sigprocmask
	addl $(3 * 4), %esp
	movl $1, %edx
2:	movl 4(%esp), %ecx
	movl 0(%esp), %eax
	movl %ebx, (0 * 4)(%ecx)
	movl %esi, (1 * 4)(%ecx)
	movl %edi, (2 * 4)(%ecx)
	movl %ebp, (3 * 4)(%ecx)
	movl %esp, (4 * 4)(%ecx)
	movl %eax, (5 * 4)(%ecx)
	movl %edx, (6 * 4)(%ecx)
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
	movl 4(%esp), %ecx
	movl 8(%esp), %eax
	testl %eax, %eax
	jnz 1f
	movl $1, %eax
1:	movl (6 * 4)(%ecx), %edx
	testl %edx, %edx
	jz 2f
	pushl %eax
	pushl %ecx
	pushl $0 # oldset
	leal (7 * 4)(%ecx), %edx
	pushl %edx # set
	pushl $SIG_SETMASK # how
	call sigprocmask
	addl $(3 * 4), %esp
	popl %ecx
	popl %eax
2:	movl (0 * 4)(%ecx), %ebx
	movl (1 * 4)(%ecx), %esi
	movl (2 * 4)(%ecx), %edi
	movl (3 * 4)(%ecx), %ebp
	movl (4 * 4)(%ecx), %esp
	movl (5 * 4)(%ecx), %edx
	movl %edx, 0(%esp)
	ret
.size siglongjmp, . - siglongjmp
