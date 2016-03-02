/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/interlock.h
 * Functions that perform non-atomic operations in an atomic manner.
 */

#ifndef SORTIX_INTERLOCK_H
#define SORTIX_INTERLOCK_H

namespace Sortix {

typedef unsigned long (*ilockfunc)(unsigned long, unsigned long);
typedef struct
{
	unsigned long o; // old value
	unsigned long n; // new value
} ilret_t;

// These atomically modifies a value and return the previous value.
ilret_t InterlockedModify(unsigned long* ptr,
                          ilockfunc f,
                          unsigned long user = 0);
ilret_t InterlockedIncrement(unsigned long* ptr);
ilret_t InterlockedDecrement(unsigned long* ptr);
ilret_t InterlockedAdd(unsigned long* ptr, unsigned long arg);
ilret_t InterlockedSub(unsigned long* ptr, unsigned long arg);

} // namespace Sortix

#endif
