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

#include <sortix/kernel/calltrace.h>
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
	Interrupt::Disable();

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

static void PanicCalltrace()
{
	Log::Print("\n");
	Calltrace::Perform();
}

static void PanicHooks()
{
	if ( doublepanic )
		return;
	if ( ENABLE_CALLTRACE )
		PanicCalltrace();
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
