/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	kb/ps2.h
	A driver for the PS2 Keyboard.

******************************************************************************/

#ifndef SORTIX_KB_PS2_H
#define SORTIX_KB_PS2_H

#include "../keyboard.h"

namespace Sortix
{
	class PS2Keyboard : public Keyboard
	{
	public:
		PS2Keyboard(uint16_t iobase, uint8_t interrupt);
		virtual ~PS2Keyboard();
		virtual int Read();
		virtual size_t GetPending() const;
		virtual bool HasPending() const;
		virtual void SetOwner(KeyboardOwner* owner, void* user);

	public:
		void OnInterrupt(CPU::InterruptRegisters* regs);

	private:
		uint8_t PopScancode();
		int DecodeScancode(uint8_t scancode);
		void UpdateLEDs(int ledval);
		bool PushKey(int key);
		int PopKey();
		void NotifyOwner();

	private:
		int* queue;
		size_t queuelength;
		size_t queueoffset;
		size_t queueused;
		KeyboardOwner* owner;
		void* ownerptr;
		uint16_t iobase;
		uint8_t interrupt;
		bool scancodeescaped;
		uint8_t leds;

	};
}

#endif

