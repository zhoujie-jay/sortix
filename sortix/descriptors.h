/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	descriptors.h
	Handles file descriptors, socket descriptors, and whatnot for each process.

*******************************************************************************/

#ifndef SORTIX_DESCRIPTORS_H
#define SORTIX_DESCRIPTORS_H

namespace Sortix
{
	class Device;

	struct DescriptorEntry
	{
		Device* dev;
		int flags;
		char* path;
	};

	class DescriptorTable
	{
	public:
		DescriptorTable();
		~DescriptorTable();

	private:
		int numdevices;
		DescriptorEntry* devices;

	public:
		int Allocate(Device* object, char* pathcopy);
		int Reserve();
		void Free(int index);
		void UseReservation(int index, Device* object, char* pathcopy);
		bool Fork(DescriptorTable* forkinto);
		void OnExecute();
		void Reset();
		void SetFlags(int index, int flags);
		int GetFlags(int index);
		const char* GetPath(int index);

	public:
		inline Device* Get(int index)
		{
			if ( !devices ) { return NULL; }
			if ( index < 0 || numdevices <= index ) { return NULL; }
			return devices[index].dev;
		}

	};
}

#endif

