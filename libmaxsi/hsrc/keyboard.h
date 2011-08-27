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

	keyboard.h
	Provides constants for various useful keys related to keyboard input.

******************************************************************************/

#ifndef LIBMAXSI_KEYBOARD_H
#define LIBMAXSI_KEYBOARD_H

namespace Maxsi
{
	namespace Keyboard
	{
		const uint32_t UNKNOWN = 0x40000000;
		const uint32_t ESC = 0x40000000 + 1;
		const uint32_t CTRL = 0x40000000 + 2;
		const uint32_t LSHFT = 0x40000000 + 3;
		const uint32_t RSHFT = 0x40000000 + 4;
		const uint32_t ALT = 0x40000000 + 5;
		const uint32_t F1 = 0x40000000 + 6;
		const uint32_t F2 = 0x40000000 + 7;
		const uint32_t F3 = 0x40000000 + 8;
		const uint32_t F4 = 0x40000000 + 9;
		const uint32_t F5 = 0x40000000 + 10;
		const uint32_t F6 = 0x40000000 + 11;
		const uint32_t F7 = 0x40000000 + 12;
		const uint32_t F8 = 0x40000000 + 13;
		const uint32_t F9 = 0x40000000 + 14;
		const uint32_t F10 = 0x40000000 + 15;
		const uint32_t F11 = 0x40000000 + 16;
		const uint32_t F12 = 0x40000000 + 17;
		const uint32_t SCRLCK = 0x40000000 + 18;
		const uint32_t HOME = 0x40000000 + 19;
		const uint32_t UP = 0x40000000 + 20;
		const uint32_t LEFT = 0x40000000 + 21;
		const uint32_t RIGHT = 0x40000000 + 22;
		const uint32_t DOWN = 0x40000000 + 23;
		const uint32_t PGUP = 0x40000000 + 24;
		const uint32_t PGDOWN = 0x40000000 + 25;
		const uint32_t END = 0x40000000 + 26;
		const uint32_t INS = 0x40000000 + 27;
		const uint32_t DEL = 0x40000000 + 28;
		const uint32_t CAPS = 0x40000000 + 29;
		const uint32_t NONE = 0x40000000 + 30;
		const uint32_t ALTGR = 0x40000000 + 31;
		const uint32_t NUMLCK = 0x40000000 + 32;
		const uint32_t SIGINT = 0x40000000 + 33;
		const uint32_t DEPRESSED = (1<<31);
	}
}

#endif
