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

#include <sortix/kernel/platform.h>
#include <sortix/kernel/string.h>
#include <assert.h>
#include <string.h>
#include "descriptors.h"
#include "device.h"
#include <sortix/fcntl.h>

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
			delete[] devices[i].path;
		}

		delete[] devices;
		devices = NULL;
		numdevices = 0;
	}

	int DescriptorTable::Allocate(Device* object, char* pathcopy)
	{
		for ( int i = 0; i < numdevices; i++ )
		{
			if ( !devices[i].dev )
			{
				object->Refer();
				devices[i].dev = object;
				devices[i].flags = 0;
				devices[i].path = pathcopy;
				return i;
			}
		}

		int newlistlength = numdevices*2;
		if ( newlistlength == 0 ) { newlistlength = 8; }

		DescriptorEntry* newlist = new DescriptorEntry[newlistlength];
		if ( newlist == NULL ) { return -1; }

		if ( devices != NULL )
		{
			memcpy(newlist, devices, sizeof(*devices) * numdevices);
		}

		for ( int i = numdevices; i < newlistlength; i++ )
			newlist[i].dev = NULL,
			newlist[i].path = NULL,
			newlist[i].flags = 0;

		delete[] devices;

		devices = newlist;
		numdevices = newlistlength;

		return Allocate(object, pathcopy);
	}

	void DescriptorTable::Free(int index)
	{
		assert(index < numdevices);
		assert(devices[index].dev);

		if ( devices[index].dev != RESERVED_DEVICE )
		{
			devices[index].dev->Unref();
			delete[] devices[index].path;
		}

		devices[index].dev = NULL;
		devices[index].path = NULL;
		devices[index].flags = 0;
	}

	int DescriptorTable::Reserve()
	{
		return Allocate(RESERVED_DEVICE, NULL);
	}

	void DescriptorTable::UseReservation(int index, Device* object, char* pathcopy)
	{
		assert(index < index);
		assert(devices[index].dev != NULL);
		assert(devices[index].dev == RESERVED_DEVICE);
		assert(devices[index].path == NULL);

		object->Refer();
		devices[index].dev = object;
		devices[index].flags = 0;
		devices[index].path = pathcopy;
	}

	bool DescriptorTable::Fork(DescriptorTable* forkinto)
	{
		DescriptorEntry* newlist = new DescriptorEntry[numdevices];
		if ( !newlist ) { return false; }

		for ( int i = 0; i < numdevices; i++ )
		{
			Device* dev = devices[i].dev;
			int flags = devices[i].flags;
			char* path = devices[i].path;
			if ( !dev || dev == RESERVED_DEVICE )
				path = NULL;
			path = path ? String::Clone(path) : NULL;
			if ( flags & FD_CLOFORK ) { dev = NULL; flags = 0; }
			newlist[i].dev = dev;
			newlist[i].flags = flags;
			if ( !dev || dev == RESERVED_DEVICE ) { continue; }
			newlist[i].dev->Refer();
			newlist[i].path = path;
		}

		assert(!forkinto->devices);

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
		assert(0 <= index && index < numdevices);
		assert(devices[index].dev);
		devices[index].flags = flags;
	}

	int DescriptorTable::GetFlags(int index)
	{
		assert(0 <= index && index < numdevices);
		assert(devices[index].dev);
		return devices[index].flags;
	}

	const char* DescriptorTable::GetPath(int index)
	{
		assert(0 <= index && index < numdevices);
		assert(devices[index].dev);
		return devices[index].path;
	}
}

