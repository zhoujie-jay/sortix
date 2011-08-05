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

	serialterminal.cpp
	A terminal on a serial line.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include "log.h"
#include "uart.h"
#include "serialterminal.h"

namespace Sortix
{
	namespace SerialTerminal
	{
		void Reset()
		{
			// Set the cursor to (0,0)
			const char InitMessage[] = "\e[H";
			UART::Write(InitMessage, Maxsi::String::Length(InitMessage));
		}

		void Init()
		{
			Reset();
		}

		size_t Print(void* /*user*/, const char* string, size_t stringlen)
		{
			UART::Write(string, stringlen);
			return stringlen;
		}
	}
}
