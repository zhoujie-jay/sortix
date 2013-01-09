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

    time.cpp
    Handles interrupts whenever some time has passed and provides useful
    time-related functions to the kernel.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/timespec.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/scheduler.h>

#include "process.h"
#include "sound.h"

#ifdef PLATFORM_SERIAL
#include "serialterminal.h"
#endif

#include "cpu.h"

#if !defined(PLATFORM_X86_FAMILY)
#error Provide a time implementation for this CPU.
#endif

namespace Sortix {
namespace Time {

struct timespec since_boot;
struct timespec since_boot_period;
uint16_t since_boot_divisor;

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

static void InitializeTimer(long frequency)
{
	long real_frequence = RealFrequencyOfFrequency(frequency);
	since_boot_divisor = DivisorOfFrequency(frequency);
	since_boot = timespec_nul();
	since_boot_period = PeriodOfFrequency(real_frequence);
	RequestIRQ0(since_boot_divisor);
}

static void OnIRQ0(CPU::InterruptRegisters* regs, void* /*user*/)
{
	since_boot = timespec_add(since_boot, since_boot_period);
	Scheduler::Switch(regs);

	// TODO: There is a horrible bug that causes Sortix to only receive
	// one IRQ0 on my laptop, but it works in virtual machines. But
	// re-requesting an addtional time seems to work. Hacky and ugly.
	static bool did_ugly_irq0_hack = false;
	if ( !did_ugly_irq0_hack )
	{
		RequestIRQ0(since_boot_divisor);
		did_ugly_irq0_hack = true;
	}
}

bool TryGet(clockid_t clock, struct timespec* time, struct timespec* res)
{
	switch ( clock )
	{
	case CLOCK_REALTIME:
		return errno = ENOTSUP, false;
	case CLOCK_MONOTONIC:
	case CLOCK_BOOT:
	case CLOCK_INIT:
		// TODO: Is is possible to implement the below as atomic operations?
		Interrupt::Disable();
		if ( time )
			*time = since_boot;
		if ( res )
			*res = since_boot_period;
		Interrupt::Enable();
		return true;
	default:
		return errno = EINVAL, false;
	}
}

struct timespec Get(clockid_t clock, struct timespec* res)
{
	struct timespec ret;
	if ( !TryGet(clock, &ret, res) )
		return timespec_nul();
	return ret;
}

uintmax_t MicrosecondsOfTimespec(struct timespec time)
{
	uintmax_t seconds = time.tv_sec;
	uintmax_t nano_seconds = time.tv_nsec;
	return seconds * 1000000 + nano_seconds / 1000;
}

// TODO: This system call is for compatibility. Provide user-space with better
// facilities such as sys_clock_gettime.
static int sys_uptime(uintmax_t* usecssinceboot)
{
	struct timespec now;
	if ( !TryGet(CLOCK_BOOT, &now, NULL) )
		return -1;
	uintmax_t ret = MicrosecondsOfTimespec(now);
	if ( !CopyToUser(usecssinceboot, &ret, sizeof(ret)) )
		return -1;
	return 0;
}

void Init()
{
	Interrupt::RegisterHandler(Interrupt::IRQ0, &OnIRQ0, NULL);
	Syscall::Register(SYSCALL_UPTIME, (void*) sys_uptime);
	InitializeTimer(100/*Hz*/);
}

} // namespace Time
} // namespace Sortix
