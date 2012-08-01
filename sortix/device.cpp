/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

	device.cpp
	A base class for all devices.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <libmaxsi/memory.h>
#include "device.h"

namespace Sortix
{
	Device::Device()
	{
		refcountlock = KTHREAD_MUTEX_INITIALIZER;
		refcount = 0;
	}

	Device::~Device()
	{

	}

	void Device::Unref()
	{
		bool shoulddelete = false;
		kthread_mutex_lock(&refcountlock);
		shoulddelete = --refcount == 0 || refcount == SIZE_MAX;
		kthread_mutex_unlock(&refcountlock);
		if ( shoulddelete )
			delete this;
	}

	void Device::Refer()
	{
		ScopedLock lock(&refcountlock);
		refcount++;
	}
}

