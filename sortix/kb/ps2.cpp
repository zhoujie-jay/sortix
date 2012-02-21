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

	kb/ps2.cpp
	A driver for the PS2 Keyboard.

******************************************************************************/

#include "../platform.h"
#include <libmaxsi/memory.h>
#include "../interrupt.h"
#include "../keyboard.h"
#include "../keycodes.h"
#include "ps2.h"

using namespace Maxsi;

namespace Sortix
{
	const uint16_t DATA = 0x0;
	const uint16_t COMMAND = 0x0;
	const uint16_t STATUS = 0x4;
	const uint8_t CMD_SETLED = 0xED;
	const uint8_t LED_SCRLCK = (1<<0);
	const uint8_t LED_NUMLCK = (1<<1);
	const uint8_t LED_CAPSLCK = (1<<2);

	void PS2Keyboard__OnInterrupt(CPU::InterruptRegisters* regs, void* user)
	{
		((PS2Keyboard*) user)->OnInterrupt(regs);
	}

	PS2Keyboard::PS2Keyboard(uint16_t iobase, uint8_t interrupt)
	{
		this->queue = NULL;
		this->queuelength = 0;
		this->queueoffset = 0;
		this->queueused = 0;
		this->owner = NULL;
		this->ownerptr = NULL;
		this->iobase = iobase;
		this->interrupt = interrupt;
		this->leds = 0;
		this->scancodeescaped = false;
		Interrupt::RegisterHandler(interrupt, PS2Keyboard__OnInterrupt, this);

		// If any scancodes were already pending, our interrupt handler will
		// never be called. Let's just discard anything pending.
		PopScancode();
	}

	PS2Keyboard::~PS2Keyboard()
	{
		Interrupt::RegisterHandler(interrupt, NULL, NULL);
		delete[] queue;
	}

	void PS2Keyboard::OnInterrupt(CPU::InterruptRegisters* /*regs*/)
	{
		uint8_t scancode = PopScancode();
		int kbkey = DecodeScancode(scancode);
		if ( !kbkey ) { return; }

		if ( !PushKey(kbkey) )
		{
			Log::PrintF("Warning: dropping keystroke due to insufficient "
			            "storage space in PS2 keyboard driver.\n");
			return;
		}

		uint8_t newleds = leds;

		if ( kbkey == KBKEY_CAPSLOCK ) { newleds ^= LED_CAPSLCK; }
		if ( kbkey == KBKEY_SCROLLLOCK ) { newleds ^= LED_SCRLCK; }
		if ( kbkey == KBKEY_NUMLOCK ) { newleds ^= LED_NUMLCK; }

		if ( newleds != leds ) { UpdateLEDs(leds = newleds); }

		NotifyOwner();
	}

	void PS2Keyboard::NotifyOwner()
	{
		if ( !owner) { return; }
		owner->OnKeystroke(this, ownerptr);
	}

	int PS2Keyboard::DecodeScancode(uint8_t scancode)
	{
		const uint8_t SCANCODE_ESCAPE = 0xE0;
		if ( scancode == SCANCODE_ESCAPE )
		{
			scancodeescaped = true;
			return 0;
		}

		int offset = (scancodeescaped) ? 0x80 : 0;
		int kbkey = scancode & 0x7F;
		if ( scancode & 0x80 ) { kbkey = -kbkey - offset; }
		else { kbkey = kbkey + offset; }

		scancodeescaped = false;

		// kbkey is now in the format specified in <sortix/keycodes.h>.

		return kbkey;
	}

	uint8_t PS2Keyboard::PopScancode()
	{
		return CPU::InPortB(iobase + DATA);
	}

	void PS2Keyboard::UpdateLEDs(int ledval)
	{
		while ( (CPU::InPortB(iobase + STATUS) & (1<<1)) != 0 ) { }
		CPU::OutPortB(iobase + COMMAND, CMD_SETLED);
		while ( (CPU::InPortB(iobase + STATUS) & (1<<1)) != 0 ) { }
		CPU::OutPortB(iobase + COMMAND, ledval);
	}

	void PS2Keyboard::SetOwner(KeyboardOwner* owner, void* user)
	{
		this->owner = owner;
		this->ownerptr = user;
		if ( queueused ) { NotifyOwner(); }
	}

	bool PS2Keyboard::PushKey(int key)
	{
		// Check if we need to allocate or resize the circular queue.
		if ( queueused == queuelength )
		{
			size_t newqueuelength = (queuelength) ? queuelength * 2 : 32UL;
			int* newqueue = new int[newqueuelength];
			if ( !newqueue ) { return false; }
			size_t elemsize = sizeof(*queue);
			size_t leadingavai = queuelength-queueoffset;
			size_t leading = (leadingavai < queueused) ? leadingavai : queueused;
			size_t trailing = queueused - leading;
			Memory::Copy(newqueue, queue + queueoffset, leading * elemsize);
			Memory::Copy(newqueue + leading, queue, trailing * elemsize);
			delete[] queue;
			queue = newqueue;
			queuelength = newqueuelength;
			queueoffset = 0;
		}

		queue[(queueoffset + queueused++) % queuelength] = key;
		return true;
	}

	int PS2Keyboard::PopKey()
	{
		if ( !queueused ) { return 0; }
		int kbkey = queue[queueoffset];
		queueoffset = (queueoffset + 1) % queuelength;
		queueused--;
		return kbkey;
	}

	int PS2Keyboard::Read()
	{
		return PopKey();
	}

	size_t PS2Keyboard::GetPending() const
	{
		return queueused;
	}

	bool PS2Keyboard::HasPending() const
	{
		return queueused;
	}
}
