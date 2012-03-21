/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	panic.cpp
	Displays an error whenever something critical happens.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include <sortix/kernel/log.h>
#include "calltrace.h"
#include <sortix/kernel/panic.h>

using namespace Maxsi;

namespace Sortix
{
#if defined(DEBUG) || defined(PANIC_SHORT)
	bool longpanic = false;
#else
	bool longpanic = true;
#endif
	bool panicing = false;
	bool doublepanic = false;

	void PanicInit()
	{
		if ( panicing )
		{
			Log::PrintF("Panic while panicing:\n");
			doublepanic = true;
			return;
		}
		if ( longpanic )
		{
			Log::Print("\e[m\e[31;40m\e[2J\e[H");
			Log::Print("                                                       _                        \n");
			Log::Print("                                                      / \\                       \n");
			Log::Print("                  /\\    /\\                           /   \\                      \n");
			Log::Print("                 /  \\  /  \\                          |   |                      \n");
			Log::Print("                /    \\/    \\                         |   |                      \n");
			Log::Print("               |  X    X    \\_______________________ /   |                      \n");
			Log::Print("               |                                         |                      \n");
			Log::Print("               | _________                               /                      \n");
			Log::Print("                \\                                       /                       \n");
			Log::Print("                  ------       ---------------      ---/                        \n");
			Log::Print("                       /       \\             /      \\                           \n");
			Log::Print("                      /         \\           /        \\                          \n");
			Log::Print("                     /           \\         /          \\                         \n");
			Log::Print("                    /_____________\\       /____________\\                        \n");
			Log::Print("                                                                                \n");
			Log::Print("                                                                                \n");
			Log::Print("                              RED MAXSI OF DEATH                                \n");
			Log::Print("                                                                                \n");
			Log::Print("A critical error occured within the kernel of the operating system and it has\n");
			Log::Print("forcefully shut down as a last resort.\n");
			Log::Print("\n");
			Log::Print("Technical information:\n");
		}
		else
		{
			Log::Print("\e[m\e[31m\e[0J");
			Log::Print("RED MAXSI OF DEATH\n");
		}
		panicing = true;
	}

	void PanicCalltrace()
	{
		Log::Print("\n");
		Calltrace::Perform();
	}

	void PanicHooks()
	{
		if ( doublepanic ) { return; }
		if ( ENABLE_CALLTRACE ) { PanicCalltrace(); }
	}

	void PanicHalt()
	{
		#ifdef JSSORTIX
		JSSortix::Exit();
		#endif
		HaltKernel();
	}

	extern "C" void Panic(const char* Error)
	{
		PanicInit();
		Log::Print(Error);
		PanicHooks();
		PanicHalt();
	}

	extern "C" void PanicF(const char* Format, ...)
	{
		PanicInit();
		va_list list;
		va_start(list, Format);
		Log::PrintFV(Format, list);
		va_end(list);
		PanicHooks();
		PanicHalt();
	}
}
