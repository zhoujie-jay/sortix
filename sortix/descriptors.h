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

	class DescriptorTable
	{
	public:
		DescriptorTable();
		~DescriptorTable();

	private:
		int numdevices; 
		Device** devices;

	public:
		int Allocate(Device* object);
		int Reserve();
		void Free(int index);
		void UseReservation(int index, Device* object);

	public:
		inline Device* Get(int index)
		{
			if ( numdevices <= index ) { return NULL; }
			return devices[index];
		}

	};
}

#endif

