/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    sortix/kernel/log.h
    A system for logging various messages to the kernel log.

*******************************************************************************/

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

extern TextBufferHandle* device_textbufhandle;
extern size_t (*device_callback)(void*, const char*, size_t);
extern size_t (*device_width)(void*);
extern size_t (*device_height)(void*);
extern void (*device_get_cursor)(void*, size_t*, size_t*);
extern bool (*device_sync)(void*);
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

} // namespace Log
} // namespace Sortix

#endif
