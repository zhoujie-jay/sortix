/*
 * Copyright (c) 2011, 2012, 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/time.h
 * Retrieving the current time.
 */

#ifndef INCLUDE_SORTIX_KERNEL_TIME_H
#define INCLUDE_SORTIX_KERNEL_TIME_H

#include <sys/cdefs.h>
#include <sys/types.h>

#include <sortix/timespec.h>

namespace Sortix {
class Clock;
class Process;
class Thread;
} // namespace Sortix

namespace Sortix {
namespace Time {

void Init();
void Start();
void OnTick(struct timespec tick_period, bool system_mode);
void InitializeProcessClocks(Process* process);
void InitializeThreadClocks(Thread* thread);
struct timespec Get(clockid_t clock);
Clock* GetClock(clockid_t clock);

} // namespace Time
} // namespace Sortix

#endif
