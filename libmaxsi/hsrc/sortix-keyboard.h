/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	sortix-keyboard.h
	Provides access to the keyboard input meant for this process. This is highly
	unportable and is very likely to be removed or changed radically.

******************************************************************************/

#ifndef LIBMAXSI_SORTIX_KEYBOARD_H
#define LIBMAXSI_SORTIX_KEYBOARD_H

#include <libmaxsi/platform.h>
#include <libmaxsi/keyboard.h>

namespace System
{
	namespace Keyboard
	{
		// Receives the next pending 32-bit unicode character from the user's
		// input device (truncated to 30-bits). Bit (1<<31) is set if the key
		// was depressed. Bit (1<<30) is used for the various constants found
		// in <libmaxsi/keyboard.h>. This is just a temporary API. Returns 0 if
		// no character point was available at this time.
		const unsigned WAIT = 0;
		const unsigned POLL = 1;
		uint32_t ReceieveKeystroke(unsigned method);
	}
}

#endif
