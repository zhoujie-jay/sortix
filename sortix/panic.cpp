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

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "log.h"
#include "panic.h"

using namespace Maxsi;

namespace Sortix
{
	bool longpanic = false;

	void PanicInit()
	{
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
		}
		else
		{
			Log::Print("\e[m\e[31m\e[0J");
			Log::Print("RED MAXSI OF DEATH\n");
		}
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
		PanicHalt();
	}

	extern "C" void PanicF(const char* Format, ...)
	{
		PanicInit();
		va_list list;
		va_start(list, Format);
		Log::PrintFV(Format, list);
		va_end(list);
		PanicHalt();
	}
}
