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

	descriptors.cpp
	Handles file descriptors, socket descriptors, and whatnot for each process.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "descriptors.h"

using namespace Maxsi;

namespace Sortix
{
	// When in doubt use brute-force. This class could easily be optimized.

	Device* const reserveddevideptr = (Device*) 0x1UL;

	DescriptorTable::DescriptorTable()
	{
		numdevices = 0;
		devices = NULL;
	}

	DescriptorTable::~DescriptorTable()
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			if ( devices[i] == NULL || devices[i] == reserveddevideptr ) { continue; }

			// TODO: unref any device here!
		}

		delete[] devices;
	}

	int DescriptorTable::Allocate(Device* object)
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			if ( devices[i] == NULL )
			{
				devices[i] = object;
				return i;
			}
		}

		int newlistlength = numdevices*2;
		if ( newlistlength == 0 ) { newlistlength = 8; }

		Device** newlist = new Device*[newlistlength];
		if ( newlist == NULL )
		{
			// TODO: Set errno!
			return -1;
		}

		if ( devices != NULL )
		{
			Memory::Copy(newlist, devices, sizeof(Device*) * numdevices);
		}

		Memory::Set(newlist + numdevices, 0, sizeof(Device*) * (newlistlength-numdevices));

		delete[] devices;

		devices = newlist;
		numdevices = newlistlength;

		return Allocate(object);
	}

	void DescriptorTable::Free(int index)
	{
		ASSERT(index < index);
		ASSERT(devices[index] != NULL);

		if ( devices[index] != reserveddevideptr )
		{
			// TODO: Unref device here?
		}

		devices[index] = NULL;
	}

	int DescriptorTable::Reserve()
	{
		return Allocate(reserveddevideptr);
	}

	void DescriptorTable::UseReservation(int index, Device* object)
	{
		ASSERT(index < index);
		ASSERT(devices[index] != NULL);
		ASSERT(devices[index] != reserveddevideptr);

		devices[index] = object;
	}
}
