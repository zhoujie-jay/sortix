/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	keyboard.cpp
	An interface to keyboards.

******************************************************************************/

#include "platform.h"
#include "interrupt.h"
#include "syscall.h"
#include "keyboard.h"
#include "kb/ps2.h"
#include "kb/layout/us.h"
#include "logterminal.h"

namespace Sortix
{
	DevTerminal* tty;

	void Keyboard::Init()
	{
		Keyboard* keyboard = new PS2Keyboard(0x60, Interrupt::IRQ1);
		if ( !keyboard ) { Panic("Could not allocate PS2 Keyboard driver"); }

		KeyboardLayout* kblayout = new KBLayoutUS;
		if ( !kblayout ) { Panic("Could not allocate keyboard layout driver"); }

		tty = new LogTerminal(keyboard, kblayout);
		if ( !tty ) { Panic("Could not allocate a simple terminal"); }
	}
}

