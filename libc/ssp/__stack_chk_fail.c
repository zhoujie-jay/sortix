/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * ssp/__stack_chk_fail.c
 * Abnormally terminate the process on stack smashing.
 */

#include <errno.h>
#include <fcntl.h>
#include <scram.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <__/wordsize.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

#if __WORDSIZE == 32
#define STACK_CHK_GUARD 0x01234567
#elif __WORDSIZE == 64
#define STACK_CHK_GUARD 0x0123456789ABCDEF
#endif

#ifdef __is_sortix_libk
/* TODO: Have this filled in by the program loader. */
#else
/* TODO: Have this filled in by the boot loader. */
#endif
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
#ifdef __is_sortix_libk
	libk_stack_chk_fail();
#else
	scram(SCRAM_STACK_SMASH, NULL);
#endif
}
