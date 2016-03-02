/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/keyboard.h
 * Various interfaces for keyboard devices and layouts.
 */

#ifndef SORTIX_KEYBOARD_H
#define SORTIX_KEYBOARD_H

#include <stddef.h>
#include <stdint.h>

namespace Sortix {

class Keyboard;
class KeyboardOwner;

class Keyboard
{
public:
	virtual ~Keyboard() { }
	virtual int Read() = 0;
	virtual size_t GetPending() const = 0;
	virtual bool HasPending() const = 0;
	virtual void SetOwner(KeyboardOwner* owner, void* user) = 0;

};

class KeyboardOwner
{
public:
	virtual ~KeyboardOwner() { }
	virtual void OnKeystroke(Keyboard* keyboard, void* user) = 0;

};

} // namespace Sortix

#endif
