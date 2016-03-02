/*
 * Copyright (c) 201 Jonas 'Sortie' Termansen.
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
 * msr/wrmsr.c
 * Sets a model specific register.
 */

#include <sys/syscall.h>

#include <stdint.h>
#include <msr.h>

#if defined(__i386__) || defined(__x86_64__)

DEFN_SYSCALL2(uint64_t, sys_wrmsr, SYSCALL_WRMSR, uint32_t, uint64_t);

uint64_t wrmsr(uint32_t msrid, uint64_t value)
{
	return sys_wrmsr(msrid, value);
}

#endif
