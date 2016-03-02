/*
 * Copyright (c) 2011, 2012, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * kb/ps2.h
 * PS2 Keyboard.
 */

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
