/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/signal.h
 * Asynchronous user-space thread interruption.
 */

#ifndef INCLUDE_SORTIX_KERNEL_SIGNAL_H
#define INCLUDE_SORTIX_KERNEL_SIGNAL_H

#include <sortix/signal.h>
#include <sortix/sigset.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/registers.h>

namespace Sortix {

extern "C" volatile unsigned long asm_signal_is_pending;

namespace Signal {

void Init();
inline bool IsPending() { return asm_signal_is_pending != 0; }
void DispatchHandler(struct interrupt_context* intctx, void* user);
void ReturnHandler(struct interrupt_context* intctx, void* user);

} // namespace Signal

} // namespace Sortix

#endif
