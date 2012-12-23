/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

#ifndef SORTIX_LOG_H
#define SORTIX_LOG_H

#include <stdio.h>
#include <string.h>

namespace Sortix
{
	namespace Log
	{
		extern size_t (*deviceCallback)(void*, const char*, size_t);
		extern size_t (*deviceWidth)(void*);
		extern size_t (*deviceHeight)(void*);
		extern bool (*deviceSync)(void*);
		extern void* devicePointer;

		void Init(size_t (*callback)(void*, const char*, size_t),
		          size_t (*widthfunc)(void*),
		          size_t (*heightfunc)(void*),
		          bool (*syncfunc)(void*),
		          void* user);

		inline void Flush()
		{
			if ( deviceCallback ) { deviceCallback(devicePointer, NULL, 0); }
		}

		inline size_t Width()
		{
			return deviceWidth(devicePointer);
		}

		inline size_t Height()
		{
			return deviceHeight(devicePointer);
		}

		inline bool Sync()
		{
			return deviceSync(devicePointer);
		}

		inline size_t Print(const char* str)
		{
			if ( !deviceCallback ) { return 0; }
			return deviceCallback(devicePointer, str, strlen(str));
		}

		inline size_t PrintData(const void* ptr, size_t size)
		{
			if ( !deviceCallback ) { return 0; }
			return deviceCallback(devicePointer, (const char*) ptr, size);
		}

		inline size_t PrintF(const char* format, ...)
		{
			va_list list;
			va_start(list, format);
			size_t result = vprintf_callback(deviceCallback, devicePointer, format, list);
			va_end(list);
			return result;
		}

		inline size_t PrintFV(const char* format, va_list list)
		{
			return vprintf_callback(deviceCallback, devicePointer, format, list);
		}
	}
}

#endif
