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

	conway.h
	THE GAME! You lost it. OF LIFE!

******************************************************************************/

#ifndef SORTIX_CONWAY_H
#define SORTIX_CONWAY_H

namespace Sortix
{
	namespace Conway
	{
		void Init();
		void Cycle();
		void Render();
		void Update(int TimePassed);
		void OnFuture();
	}
}

#endif

