/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

	log.h
	A system for logging various messages to the kernel log.

******************************************************************************/

#ifndef SORTIX_LOG_H
#define SORTIX_LOG_H

#include <libmaxsi/string.h>
#include <libmaxsi/format.h>

namespace Sortix
{
	namespace Log
	{
		extern Maxsi::Format::Callback deviceCallback;
		extern void* devicePointer;

		void Init(Maxsi::Format::Callback callback, void* user);

		inline void Flush()
		{
			if ( deviceCallback ) { deviceCallback(devicePointer, NULL, 0); }
		}

		inline size_t Print(const char* str)
		{
			using namespace Maxsi;
			if ( !deviceCallback ) { return 0; }
			return deviceCallback(devicePointer, str, String::Length(str));
		}

		inline size_t PrintData(const void* ptr, size_t size)
		{
			if ( !deviceCallback ) { return 0; }
			return deviceCallback(devicePointer, (const char*) ptr, size);
		}

		inline size_t PrintF(const char* format, ...)
		{
			using namespace Maxsi;
			va_list list;
			va_start(list, format);
			size_t result = Format::Virtual(deviceCallback, devicePointer, format, list);
			va_end(list);
			return result;
		}

		inline size_t PrintFV(const char* format, va_list list)
		{
			using namespace Maxsi;
			return Format::Virtual(deviceCallback, devicePointer, format, list);
		}
	}
}

#endif

