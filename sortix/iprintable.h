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

	iprintable.h
	A common interface shared by all devices that can be printed text to.

******************************************************************************/

#ifndef SORTIX_IPRINTABLE_H
#define SORTIX_IPRINTABLE_H

namespace Sortix
{
	int UInt8ToString(uint8_t Num, char* Dest);
	int UInt16ToString(uint16_t Num, char* Dest);
	int UInt32ToString(uint32_t Num, char* Dest);
	int UInt64ToString(uint64_t Num, char* Dest);

	class IPrintable
	{
	public:
		size_t PrintFV(const char* Format, va_list Parameters);
		size_t PrintF(const char* Format, ...);

	public:
		virtual size_t Print(const char* Message) = 0;

	};
}

#endif

