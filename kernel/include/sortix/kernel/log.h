/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/log.h
 * A system for logging various messages to the kernel log.
 */

#ifndef INCLUDE_SORTIX_KERNEL_LOG_H
#define INCLUDE_SORTIX_KERNEL_LOG_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct multiboot_info multiboot_info_t;

namespace Sortix {

class TextBufferHandle;

} // namespace Sortix

namespace Sortix {
namespace Log {

extern uint8_t* fallback_framebuffer;
extern size_t fallback_framebuffer_bpp;
extern size_t fallback_framebuffer_pitch;
extern size_t fallback_framebuffer_width;
extern size_t fallback_framebuffer_height;

extern TextBufferHandle* device_textbufhandle;
extern size_t (*device_callback)(void*, const char*, size_t);
extern size_t (*device_width)(void*);
extern size_t (*device_height)(void*);
extern void (*device_get_cursor)(void*, size_t*, size_t*);
extern bool (*device_sync)(void*);
extern void (*device_invalidate)(void*);
extern void* device_pointer;
extern bool (*emergency_device_is_impaired)(void*);
extern bool (*emergency_device_recoup)(void*);
extern void (*emergency_device_reset)(void*);
extern size_t (*emergency_device_callback)(void*, const char*, size_t);
extern size_t (*emergency_device_width)(void*);
extern size_t (*emergency_device_height)(void*);
extern void (*emergency_device_get_cursor)(void*, size_t*, size_t*);
extern bool (*emergency_device_sync)(void*);
extern void* emergency_device_pointer;

inline void Flush()
{
	device_callback(device_pointer, NULL, 0);
}

inline size_t Width()
{
	return device_width(device_pointer);
}

inline size_t Height()
{
	return device_height(device_pointer);
}

inline void GetCursor(size_t* col, size_t* row)
{
	return device_get_cursor(device_pointer, col, row);
}

inline bool Sync()
{
	return device_sync(device_pointer);
}

inline void Invalidate()
{
	return device_invalidate(device_pointer);
}

inline size_t Print(const char* str)
{
	return device_callback(device_pointer, str, strlen(str));
}

inline size_t PrintData(const void* ptr, size_t size)
{
	return device_callback(device_pointer, (const char*) ptr, size);
}

__attribute__((format(printf, 1, 2)))
inline size_t PrintF(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	int result = vcbprintf(device_pointer, device_callback, format, list);
	va_end(list);
	if ( result < 0 )
		return SIZE_MAX;
	return (size_t) result;
}

__attribute__((format(printf, 1, 0)))
inline size_t PrintFV(const char* format, va_list list)
{
	int result = vcbprintf(device_pointer, device_callback, format, list);
	if ( result < 0 )
		return SIZE_MAX;
	return (size_t) result;
}

void Init(multiboot_info_t* bootinfo);
void Center(const char* string);

} // namespace Log
} // namespace Sortix

#endif
