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
 * x86-family/time.cpp
 * Retrieving the current time.
 */

#include <sys/types.h>

#include <timespec.h>

#include <sortix/timespec.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/cpu.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/time.h>

namespace Sortix {
namespace Time {

static uint16_t DivisorOfFrequency(long frequency)
{
	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Note that the divisor
	// must be small enough to fit into 16 bits.
	return 1193180 / frequency;
}

static long FrequencyOfDivisor(uint16_t divisor)
{
	return 1193180 / divisor;
}

static long RealFrequencyOfFrequency(long frequency)
{
	return FrequencyOfDivisor(DivisorOfFrequency(frequency));
}

static struct timespec PeriodOfFrequency(long frequency)
{
	long period_ns = 1000000000L / frequency;
	return timespec_make(0, period_ns);
}

static void RequestIRQ0(uint16_t divisor)
{
	outport8(0x43, 0x36);
	outport8(0x40, divisor >> 0 & 0xFF);
	outport8(0x40, divisor >> 8 & 0xFF);
}

extern Clock* realtime_clock;
extern Clock* uptime_clock;

struct interrupt_handler timer_interrupt_registration;

static struct timespec tick_period;
static long tick_frequency;
static uint16_t tick_divisor;

static void OnIRQ0(struct interrupt_context* intctx, void* /*user*/)
{
	OnTick(tick_period, !InUserspace(intctx));
	Scheduler::Switch(intctx);

	// TODO: There is a horrible bug that causes Sortix to only receive
	//       one IRQ0 on my laptop, but it works in virtual machines. But
	//       re-requesting an addtional time seems to work. Hacky and ugly.
	// TODO: Confirm whether this still happens and whether it is trigged by
	//       another bug in my system.
	static bool did_ugly_irq0_hack = false;
	if ( !did_ugly_irq0_hack )
		RequestIRQ0(tick_divisor),
		did_ugly_irq0_hack = true;
}

void CPUInit()
{
	// Estimate the rate that interrupts will be coming at.
	long desired_frequency = 100/*Hz*/;
	tick_frequency = RealFrequencyOfFrequency(desired_frequency);
	tick_divisor = DivisorOfFrequency(tick_frequency);
	tick_period = PeriodOfFrequency(tick_frequency);

	// Initialize the clocks on this system.
	realtime_clock->SetCallableFromInterrupts(true);
	uptime_clock->SetCallableFromInterrupts(true);
	struct timespec nul_time = timespec_nul();
	realtime_clock->Set(&nul_time, &tick_period);
	uptime_clock->Set(&nul_time, &tick_period);
}

void InitializeProcessClocks(Process* process)
{
	struct timespec nul_time = timespec_nul();
	process->execute_clock.SetCallableFromInterrupts(true);
	process->execute_clock.Set(&nul_time, &tick_period);
	process->system_clock.SetCallableFromInterrupts(true);
	process->system_clock.Set(&nul_time, &tick_period);
	process->child_execute_clock.Set(&nul_time, &tick_period);
	process->child_execute_clock.SetCallableFromInterrupts(true);
	process->child_system_clock.Set(&nul_time, &tick_period);
	process->child_system_clock.SetCallableFromInterrupts(true);
}

void InitializeThreadClocks(Thread* thread)
{
	struct timespec nul_time = timespec_nul();
	thread->execute_clock.SetCallableFromInterrupts(true);
	thread->execute_clock.Set(&nul_time, &tick_period);
	thread->system_clock.SetCallableFromInterrupts(true);
	thread->system_clock.Set(&nul_time, &tick_period);
}

void Start()
{
	// Handle timer interrupts if they arrive.
	timer_interrupt_registration.handler = OnIRQ0;
	timer_interrupt_registration.context = 0;
	Interrupt::RegisterHandler(Interrupt::IRQ0, &timer_interrupt_registration);

	// Request a timer interrupt now that we can handle them safely.
	RequestIRQ0(tick_divisor);
}

} // namespace Time
} // namespace Sortix
