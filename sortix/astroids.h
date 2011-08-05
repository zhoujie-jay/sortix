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

	astroids.h
	IN SPACE, NO ONE CAN HEAR YOU find ~ | xargs shred

******************************************************************************/

#ifndef SORTIX_ASTROIDS_H
#define SORTIX_ASTROIDS_H

namespace Sortix
{
	class Astroids : public IKeystrokable, public ITimerable
	{
	private:
		nat Time;
		nat NextUpdate;

	private:
		int PX;
		int PY;
		int TrailX;
		int TrailY;
		int TrailLen;
		int TrailMaxLen;
		int PVX;
		int PVY;

	public:
		void Init();
	
	private:
		void ClearScreen();
		void Reset();
		void Update();

	public:
		virtual void OnKeystroke(uint32_t CodePoint, bool KeyUp);
		virtual void OnFuture(uint32_t Miliseconds);

	};

	DECLARE_GLOBAL_OBJECT(Astroids, Astroids);
}

#endif

