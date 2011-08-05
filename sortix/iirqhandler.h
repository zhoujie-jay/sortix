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

	iirqhandler.h
	An interface for classes able to handle IRQs.

	TODO: This is stupid. Get rid of this header and put the declaration
	someplace more intelligent.

******************************************************************************/

#ifndef SORTIX_IIRQHANDLER_H
#define SORTIX_IIRQHANDLER_H

namespace Sortix
{
	typedef void (*InterruptHandler)(CPU::InterruptRegisters* Registers);
}

#endif

