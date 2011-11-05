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

	x86-family.h
	CPU stuff for the x86 CPU family.

******************************************************************************/

#ifndef SORTIX_X86_FAMILY_H
#define SORTIX_X86_FAMILY_H

namespace Sortix
{
	namespace CPU
	{
		void OutPortB(uint16_t Port, uint8_t Value);
		void OutPortW(uint16_t Port, uint16_t Value);
		void OutPortL(uint16_t Port, uint32_t Value);
		uint8_t InPortB(uint16_t Port);
		uint16_t InPortW(uint16_t Port);
		uint32_t InPortL(uint16_t Port);
		void Reboot();
		void ShutDown();
	}
}

#endif

