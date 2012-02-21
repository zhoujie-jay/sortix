/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	ks/layout/us.h
	The United States keyboard layout.

******************************************************************************/

#ifndef SORTIX_KB_LAYOUT_US_H
#define SORTIX_KB_LAYOUT_US_H

#include "../../keyboard.h"

namespace Sortix
{
	class KBLayoutUS : public KeyboardLayout
	{
	public:
		KBLayoutUS();
		virtual ~KBLayoutUS();
		virtual uint32_t Translate(int kbkey);
	
	public:
		bool ProcessModifier(int kbkey, int modkey, unsigned flag);

	private:
		unsigned modifiers;

	};
}

#endif
