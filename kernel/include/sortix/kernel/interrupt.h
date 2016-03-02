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
 * sortix/kernel/interrupt.h
 * High level interrupt services.
 */

#ifndef INCLUDE_SORTIX_KERNEL_INTERRUPT_H
#define INCLUDE_SORTIX_KERNEL_INTERRUPT_H

#include <stddef.h>

#include <sortix/kernel/decl.h>
#include <sortix/kernel/registers.h>

namespace Sortix {

struct interrupt_context;

struct interrupt_handler
{
	void (*handler)(struct interrupt_context*, void*);
	void* context;
	struct interrupt_handler* next;
	struct interrupt_handler* prev;
};

} // namespace Sortix

namespace Sortix {

namespace Interrupt {

#if defined(__i386__) || defined(__x86_64__)
const unsigned int IRQ0 = 32;
const unsigned int IRQ1 = 33;
const unsigned int IRQ2 = 34;
const unsigned int IRQ3 = 35;
const unsigned int IRQ4 = 36;
const unsigned int IRQ5 = 37;
const unsigned int IRQ6 = 38;
const unsigned int IRQ7 = 39;
const unsigned int IRQ8 = 40;
const unsigned int IRQ9 = 41;
const unsigned int IRQ10 = 42;
const unsigned int IRQ11 = 43;
const unsigned int IRQ12 = 44;
const unsigned int IRQ13 = 45;
const unsigned int IRQ14 = 46;
const unsigned int IRQ15 = 47;
#endif

extern "C" unsigned long asm_is_cpu_interrupted;

inline bool IsEnabled()
{
#if defined(__i386__) || defined(__x86_64__)
	unsigned long is_enabled;
	asm("pushf\t\n"
	    "pop %0\t\n"
	    "and $0x000200, %0" : "=r"(is_enabled));
	return is_enabled != 0;
#else
#warning "You need to implement checking if interrupts are on"
#endif
}

inline void Enable()
{
#if defined(__i386__) || defined(__x86_64__)
	asm volatile("sti");
#else
#warning "You need to implement enabling interrupts"
#endif
}

inline void Disable()
{
#if defined(__i386__) || defined(__x86_64__)
	asm volatile("cli");
#else
#warning "You need to implement disabling interrupts"
#endif
}

inline bool IsCPUInterrupted()
{
	return asm_is_cpu_interrupted != 0;
}

inline bool SetEnabled(bool is_enabled)
{
	bool wasenabled = IsEnabled();
	if ( is_enabled )
		Enable();
	else
		Disable();
	return wasenabled;
}

void RegisterHandler(unsigned int index, struct interrupt_handler* handler);
void UnregisterHandler(unsigned int index, struct interrupt_handler* handler);

void Init();
void InitWorker();
void WorkerThread(void* user);

bool ScheduleWork(void (*handler)(void*, void*, size_t),
                  void* handler_context,
                  void* payload,
                  size_t payload_size);

} // namespace Interrupt
} // namespace Sortix

#endif
