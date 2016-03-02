/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/ps2.h
 * Various interfaces for keyboard devices and layouts.
 */

#ifndef INCLUDE_SORTIX_KERNEL_PS2_H
#define INCLUDE_SORTIX_KERNEL_PS2_H

#include <stdint.h>

namespace Sortix {

class PS2Device
{
public:
	virtual ~PS2Device() { }
	virtual void PS2DeviceInitialize(void* send_ctx, bool (*send)(void*, uint8_t),
                                     uint8_t* id, size_t id_size) = 0;
	virtual void PS2DeviceOnByte(uint8_t byte) = 0;

};

} // namespace Sortix

#endif
