/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	terminal.h
	Reads data from an input source (such as a keyboard), optionally does line-
	buffering, and redirects data to an output device (such as the VGA).

******************************************************************************/

#ifndef SORTIX_TERMINAL_H
#define SORTIX_TERMINAL_H

#include "device.h"
#include "stream.h"
#include "termmode.h"

namespace Sortix
{
	class DevTerminal : public DevStream
	{
	public:
		typedef DevStream BaseClass;

	public:
		virtual bool IsType(unsigned type) const
		{
			return type == Device::TERMINAL || BaseClass::IsType(type);
		}

	public:
		virtual bool SetMode(unsigned mode) = 0;
		virtual bool SetWidth(unsigned width) = 0;
		virtual bool SetHeight(unsigned height) = 0;
		virtual unsigned GetMode() const = 0;
		virtual unsigned GetWidth() const = 0;
		virtual unsigned GetHeight() const = 0;

	};

	namespace Terminal
	{
		void Init();
	}
}
#endif

