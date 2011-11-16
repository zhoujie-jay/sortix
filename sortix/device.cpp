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

	device.cpp
	A base class for all devices.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/memory.h>
#include "device.h"

namespace Sortix
{
	Device::Device()
	{
		refcount = 0;
	}

	Device::~Device()
	{
		
	}

	void Device::Unref()
	{
		if ( --refcount == 0 ) { delete this; }
	}

	void Device::Refer()
	{
		refcount++;
	}
}

