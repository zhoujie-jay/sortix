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

	descriptors.h
	Handles file descriptors, socket descriptors, and whatnot for each process.

******************************************************************************/

#ifndef SORTIX_DESCRIPTORS_H
#define SORTIX_DESCRIPTORS_H

namespace Sortix
{
	class Device;

	struct Description
	{
		nat type;
		Device* object;
	};

	class DescriptorTable
	{
	public:
		DescriptorTable();
		~DescriptorTable();

	private:
		int _numDescs 
		Description* _descs;

	public:
		int Reserve();
		void UseReservation(int index, Device* object);
		int Allocate(Device* object;
		bool Free(int index);

	public:
		inline Description* get(int index)
		{
			if ( _numDescs <= index ) { return NULL; }
			return _descs + index;
		}
	};
}

#endif

