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

	serialterminal.h
	A terminal on a serial line.

*******************************************************************************/

#ifndef SORTIX_SERIALTERMINAL_H
#define SORTIX_SERIALTERMINAL_H

namespace Sortix
{
	namespace SerialTerminal
	{
		void Init();
		void Reset();
		void OnTick();
		void OnVGAModified();
		size_t Print(void* user, const char* string, size_t stringlen);
	}
}

#endif
