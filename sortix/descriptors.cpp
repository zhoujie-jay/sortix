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

	descriptors.cpp
	Handles file descriptors, socket descriptors, and whatnot for each process.

*******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "descriptors.h"
#include "device.h"
#include <sortix/fcntl.h>

using namespace Maxsi;

namespace Sortix
{
	// When in doubt use brute-force. This class could easily be optimized.

	Device* const RESERVED_DEVICE = (Device*) 0x1UL;

	DescriptorTable::DescriptorTable()
	{
		numdevices = 0;
		devices = NULL;
	}

	DescriptorTable::~DescriptorTable()
	{
		Reset();
	}

	void DescriptorTable::Reset()
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			Device* dev = devices[i].dev;
			if ( !dev || dev == RESERVED_DEVICE ) { continue; }

			dev->Unref();
		}

		delete[] devices;
		devices = NULL;
		numdevices = 0;
	}

	int DescriptorTable::Allocate(Device* object)
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			if ( !devices[i].dev )
			{
				object->Refer();
				devices[i].dev = object;
				devices[i].flags = 0;
				return i;
			}
		}

		int newlistlength = numdevices*2;
		if ( newlistlength == 0 ) { newlistlength = 8; }

		DescriptorEntry* newlist = new DescriptorEntry[newlistlength];
		if ( newlist == NULL ) { return -1; }

		if ( devices != NULL )
		{
			Memory::Copy(newlist, devices, sizeof(*devices) * numdevices);
		}

		size_t numpadded = newlistlength-numdevices;
		Memory::Set(newlist + numdevices, 0, sizeof(*devices) * numpadded);

		delete[] devices;

		devices = newlist;
		numdevices = newlistlength;

		return Allocate(object);
	}

	void DescriptorTable::Free(int index)
	{
		ASSERT(index < numdevices);
		ASSERT(devices[index].dev);

		if ( devices[index].dev != RESERVED_DEVICE )
		{
			devices[index].dev->Unref();
		}

		devices[index].dev = NULL;
		devices[index].flags = 0;
	}

	int DescriptorTable::Reserve()
	{
		return Allocate(RESERVED_DEVICE);
	}

	void DescriptorTable::UseReservation(int index, Device* object)
	{
		ASSERT(index < index);
		ASSERT(devices[index].dev != NULL);
		ASSERT(devices[index].dev == RESERVED_DEVICE);

		object->Refer();
		devices[index].dev = object;
		devices[index].flags = 0;
	}

	bool DescriptorTable::Fork(DescriptorTable* forkinto)
	{
		DescriptorEntry* newlist = new DescriptorEntry[numdevices];
		if ( !newlist ) { return false; }

		for ( int i = 0; i < numdevices; i++ )
		{
			Device* dev = devices[i].dev;
			int flags = devices[i].flags;
			if ( flags & FD_CLOFORK ) { dev = NULL; flags = 0; }
			newlist[i].dev = dev;
			newlist[i].flags = flags;
			if ( !dev || dev == RESERVED_DEVICE ) { continue; }
			newlist[i].dev->Refer();
		}

		ASSERT(!forkinto->devices);

		forkinto->devices = newlist;
		forkinto->numdevices = numdevices;

		return true;
	}

	void DescriptorTable::OnExecute()
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			if ( devices[i].flags & FD_CLOEXEC ) { Free(i); }
		}
	}

	void DescriptorTable::SetFlags(int index, int flags)
	{
		ASSERT(0 <= index && index < numdevices);
		ASSERT(devices[index].dev);
		devices[index].flags = flags;
	}

	int DescriptorTable::GetFlags(int index)
	{
		ASSERT(0 <= index && index < numdevices);
		ASSERT(devices[index].dev);
		return devices[index].flags;
	}
}

