/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014, 2015.

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

    kb/ps2.h
    PS2 Keyboard.

*******************************************************************************/

#ifndef SORTIX_KB_PS2_H
#define SORTIX_KB_PS2_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/ps2.h>

namespace Sortix {

class PS2Keyboard : public Keyboard, public PS2Device
{
public:
	PS2Keyboard();
	virtual ~PS2Keyboard();
	virtual int Read();
	virtual size_t GetPending() const;
	virtual bool HasPending() const;
	virtual void SetOwner(KeyboardOwner* owner, void* user);
	virtual void PS2DeviceInitialize(void* send_ctx, bool (*send)(void*, uint8_t),
                                     uint8_t* id, size_t id_size);
	virtual void PS2DeviceOnByte(uint8_t byte);

private:
	void OnKeyboardKey(int kbkey);
	void UpdateLEDs(int ledval);
	bool PushKey(int key);
	int PopKey();
	void NotifyOwner();

private:
	mutable kthread_mutex_t kblock;
	int* queue;
	size_t queuelength;
	size_t queueoffset;
	size_t queueused;
	KeyboardOwner* owner;
	void* ownerptr;
	void* send_ctx;
	bool (*send)(void*, uint8_t);
	enum
	{
		STATE_INIT = 0,
		STATE_RESET_LED,
		STATE_RESET_TYPEMATIC,
		STATE_ENABLE_SCAN,
		STATE_NORMAL,
		STATE_NORMAL_ESCAPED,
	} state;
	size_t tries;
	uint8_t leds;
	uint8_t id[2];
	size_t id_size;

};

} // namespace Sortix

#endif
