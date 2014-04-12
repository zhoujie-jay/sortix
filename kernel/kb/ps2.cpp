/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    kb/ps2.cpp
    A driver for the PS2 Keyboard.

*******************************************************************************/

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__x86_64__)
#include <msr.h>
#endif

#include <sortix/keycodes.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/debugger.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/thread.h>

#if defined(__i386__)
#include "../x86-family/gdt.h"
#endif

#include "ps2.h"

namespace Sortix {

const uint16_t DATA = 0x0;
const uint16_t COMMAND = 0x0;
const uint16_t STATUS = 0x4;
const uint8_t CMD_SETLED = 0xED;
const uint8_t LED_SCRLCK = 1 << 0;
const uint8_t LED_NUMLCK = 1 << 1;
const uint8_t LED_CAPSLCK = 1 << 2;

void PS2Keyboard__OnInterrupt(struct interrupt_context* intctx, void* user)
{
	((PS2Keyboard*) user)->OnInterrupt(intctx);
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
	this->kblock = KTHREAD_MUTEX_INITIALIZER;
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

struct PS2KeyboardWork
{
	uint8_t scancode;
};

static void PS2Keyboard__InterruptWork(void* kb_ptr, void* payload, size_t size)
{
	assert(size == sizeof(PS2KeyboardWork));
	PS2KeyboardWork* work = (PS2KeyboardWork*) payload;
	((PS2Keyboard*) kb_ptr)->InterruptWork(work->scancode);
}

void PS2Keyboard::OnInterrupt(struct interrupt_context* intctx)
{
	uint8_t scancode = PopScancode();
	if ( scancode == KBKEY_F10 )
	{
		Scheduler::SaveInterruptedContext(intctx, &CurrentThread()->registers);
		Debugger::Run(true);
	}
	PS2KeyboardWork work;
	work.scancode = scancode;
	Interrupt::ScheduleWork(PS2Keyboard__InterruptWork, this, &work, sizeof(work));
}

void PS2Keyboard::InterruptWork(uint8_t scancode)
{
	kthread_mutex_lock(&kblock);
	int kbkey = DecodeScancode(scancode);
	if ( !kbkey )
	{
		kthread_mutex_unlock(&kblock);
		return;
	}

	if ( !PushKey(kbkey) )
	{
		Log::PrintF("Warning: dropping keystroke due to insufficient "
		            "storage space in PS2 keyboard driver.\n");
		kthread_mutex_unlock(&kblock);
		return;
	}

	uint8_t newleds = leds;

	if ( kbkey == KBKEY_CAPSLOCK )
		newleds ^= LED_CAPSLCK;
	if ( kbkey == KBKEY_SCROLLLOCK )
		newleds ^= LED_SCRLCK;
	if ( kbkey == KBKEY_NUMLOCK )
		newleds ^= LED_NUMLCK;

	if ( newleds != leds )
		UpdateLEDs(leds = newleds);

	kthread_mutex_unlock(&kblock);

	NotifyOwner();
}

void PS2Keyboard::NotifyOwner()
{
	if ( !owner)
		return;
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

	int offset = scancodeescaped ? 0x80 : 0;
	int kbkey = scancode & 0x7F;
	kbkey = scancode & 0x80 ? -kbkey - offset : kbkey + offset;

	scancodeescaped = false;

	// kbkey is now in the format specified in <sortix/keycodes.h>.

	return kbkey;
}

uint8_t PS2Keyboard::PopScancode()
{
	return inport8(iobase + DATA);
}

void PS2Keyboard::UpdateLEDs(int ledval)
{
	while ( (inport8(iobase + STATUS) & (1<<1)) );
	outport8(iobase + COMMAND, CMD_SETLED);
	while ( (inport8(iobase + STATUS) & (1<<1)) );
	outport8(iobase + COMMAND, ledval);
}

void PS2Keyboard::SetOwner(KeyboardOwner* owner, void* user)
{
	kthread_mutex_lock(&kblock);
	this->owner = owner;
	this->ownerptr = user;
	kthread_mutex_unlock(&kblock);
	if ( queueused )
		NotifyOwner();
}

bool PS2Keyboard::PushKey(int key)
{
	// Check if we need to allocate or resize the circular queue.
	if ( queueused == queuelength )
	{
		size_t newqueuelength = (queuelength) ? queuelength * 2 : 32UL;
		int* newqueue = new int[newqueuelength];
		if ( !newqueue )
			return false;
		size_t elemsize = sizeof(*queue);
		size_t leadingavai = queuelength-queueoffset;
		size_t leading = (leadingavai < queueused) ? leadingavai : queueused;
		size_t trailing = queueused - leading;
		memcpy(newqueue, queue + queueoffset, leading * elemsize);
		memcpy(newqueue + leading, queue, trailing * elemsize);
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
	if ( !queueused )
		return 0;
	int kbkey = queue[queueoffset];
	queueoffset = (queueoffset + 1) % queuelength;
	queueused--;
	return kbkey;
}

int PS2Keyboard::Read()
{
	ScopedLock lock(&kblock);
	return PopKey();
}

size_t PS2Keyboard::GetPending() const
{
	ScopedLock lock(&kblock);
	return queueused;
}

bool PS2Keyboard::HasPending() const
{
	ScopedLock lock(&kblock);
	return queueused;
}

} // namespace Sortix
