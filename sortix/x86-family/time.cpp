/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    x86-family/time.cpp
    Retrieving the current time.

*******************************************************************************/

#include <sys/types.h>

#include <timespec.h>

#include <sortix/timespec.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/clock.h>
#include <sortix/kernel/cpu.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
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
	CPU::OutPortB(0x43, 0x36);
	CPU::OutPortB(0x40, divisor >> 0 & 0xFF);
	CPU::OutPortB(0x40, divisor >> 8 & 0xFF);
}

extern Clock* realtime_clock;
extern Clock* uptime_clock;

static struct timespec tick_period;
static long tick_frequency;
static uint16_t tick_divisor;

static void OnIRQ0(CPU::InterruptRegisters* regs, void* /*user*/)
{
	OnTick(tick_period, !regs->InUserspace());
	Scheduler::Switch(regs);

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

void Start()
{
	// Handle timer interrupts if they arrive.
	Interrupt::RegisterHandler(Interrupt::IRQ0, &OnIRQ0, NULL);

	// Request a timer interrupt now that we can handle them safely.
	RequestIRQ0(tick_divisor);
}

} // namespace Time
} // namespace Sortix
