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

	event.h
	Each thread can wait for an event to happen and be signaled when it does.

******************************************************************************/

#ifndef SORTIX_EVENT_H
#define SORTIX_EVENT_H

namespace Sortix
{
	class Thread;

	class Event
	{
	public:
		Event();
		~Event();

	public:
		void Register();
		void Signal();
		void Unregister(Thread* thread);

	private:
		Thread* waiting;

	};
}

#endif

