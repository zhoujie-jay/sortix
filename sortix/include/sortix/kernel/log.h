/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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
#include <string.h>

namespace Sortix {
namespace Log {

extern size_t (*device_callback)(void*, const char*, size_t);
extern size_t (*device_width)(void*);
extern size_t (*device_height)(void*);
extern bool (*device_sync)(void*);
extern void* device_pointer;
extern bool (*emergency_device_is_impaired)(void*);
extern bool (*emergency_device_recoup)(void*);
extern void (*emergency_device_reset)(void*);
extern size_t (*emergency_device_callback)(void*, const char*, size_t);
extern size_t (*emergency_device_width)(void*);
extern size_t (*emergency_device_height)(void*);
extern bool (*emergency_device_sync)(void*);
extern void* emergency_device_pointer;

inline void Flush()
{
	if ( !device_callback )
		return;
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

inline bool Sync()
{
	return device_sync(device_pointer);
}

inline size_t Print(const char* str)
{
	if ( !device_callback )
		return 0;
	return device_callback(device_pointer, str, strlen(str));
}

inline size_t PrintData(const void* ptr, size_t size)
{
	if ( !device_callback )
		return 0;
	return device_callback(device_pointer, (const char*) ptr, size);
}

inline size_t PrintF(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	size_t result = vprintf_callback(device_callback, device_pointer, format, list);
	va_end(list);
	return result;
}

inline size_t PrintFV(const char* format, va_list list)
{
	return vprintf_callback(device_callback, device_pointer, format, list);
}

} // namespace Log
} // namespace Sortix

#endif
