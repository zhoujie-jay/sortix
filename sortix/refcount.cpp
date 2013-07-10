/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    refcount.cpp
    A class that implements reference counting.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <assert.h>

namespace Sortix {

Refcounted::Refcounted()
{
	reflock = KTHREAD_MUTEX_INITIALIZER;
	refcount = 1;
}

Refcounted::~Refcounted()
{
	// It's OK to be deleted if our refcount is 1, it won't mess with any
	// other owners that might need us.
	assert(refcount <= 1);
}

void Refcounted::Refer()
{
	ScopedLock lock(&reflock);
	refcount++;
}

void Refcounted::Unref()
{
	kthread_mutex_lock(&reflock);
	bool deleteme =  !--refcount;
	kthread_mutex_unlock(&reflock);
	if ( deleteme )
		delete this;
}

} // namespace Sortix
