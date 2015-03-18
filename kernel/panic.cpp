/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013.

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

    panic.cpp
    Displays an error whenever something critical happens.

*******************************************************************************/

#include <brand.h>
#include <string.h>

#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>

namespace Sortix {

#if defined(PANIC_SHORT)
const bool longpanic = false;
#else
const bool longpanic = true;
#endif

static bool panicing = false;
static bool doublepanic = false;
static bool logrecovering = false;

static void PanicLogoLong()
{
	Log::PrintF(BRAND_PANIC_LONG);
}

static void PanicLogoShort()
{
	Log::Print(BRAND_PANIC_SHORT);
}

void PanicInit()
{
	// This is a kernel emergency. We will need to disable preemption, such that
	// this is the only thread running. This means that we cannot acquire locks
	// and the data protected by them may be inconsistent.
	Interrupt::Disable();

	// Detect if we are really early and we don't even have a log yet.
	if ( !Log::device_callback )
		HaltKernel();

	// Detect whether a panic happened during the log recovery.
	if ( logrecovering )
	{
		// Oh no! We paniced during the log recovery that we will do momentarily
		// - this means that there probably isn't anything we can do but halt.
		HaltKernel();
	}
	logrecovering = true;

	// The kernel log normally uses locks internally and the console may be
	// rendered by a background thread. This means that we cannot use the normal
	// kernel log, but that we rather need to switch to the kernel emergency
	// log, which is able to cope with the potential inconsistencies.

	Log::device_callback = Log::emergency_device_callback;
	Log::device_width = Log::emergency_device_width;
	Log::device_height = Log::emergency_device_height;
	Log::device_get_cursor = Log::emergency_device_get_cursor;
	Log::device_sync = Log::emergency_device_sync;
	Log::device_pointer = Log::emergency_device_pointer;

	// Check whether the panic condition left the kernel log unharmed.
	if ( !Log::emergency_device_is_impaired(Log::emergency_device_pointer) )
	{
		// The kernel log device transitioned ideally to the emergency state.
	}

	// Attempt to repair inconsistent state of the emergency log device.
	else if ( Log::emergency_device_recoup(Log::emergency_device_pointer) )
	{
		// The kernel log was successfully repaired and is ready for use in the
		// current emergency state.
	}

	// It was not possible to repair the emergency device properly, so instead
	// we will need to perform a hard reset of the emergency device.
	else
	{
		Log::emergency_device_reset(Log::emergency_device_pointer);
		// The kernel log was successfully repaired and is ready for use in the
		// current emergency state.
	}

	// Handle the case where the panic code caused another system crash.
	if ( panicing )
	{
		Log::PrintF("Panic while panicing:\n");
		doublepanic = true;
		return;
	}
	panicing = true;

	// Render a notice that the system has crashed and forcefully shut down.
	longpanic ? PanicLogoLong() : PanicLogoShort();
}

static void PanicHooks()
{
	if ( doublepanic )
		return;
}

extern "C" __attribute__((noreturn)) void Panic(const char* error)
{
	PanicInit();
	Log::Print(error);
	PanicHooks();
	HaltKernel();
}

extern "C" __attribute__((noreturn)) void PanicF(const char* format, ...)
{
	PanicInit();
	va_list list;
	va_start(list, format);
	Log::PrintFV(format, list);
	va_end(list);
	PanicHooks();
	HaltKernel();
}

} // namespace Sortix
