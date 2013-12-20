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

#include <assert.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

Refcountable::Refcountable()
{
	reflock = KTHREAD_MUTEX_INITIALIZER;
	refcount = 0;
	being_deleted = false;
}

Refcountable::~Refcountable()
{
	// It's OK to be deleted if our refcount is 1, it won't mess with any
	// other owners that might need us.
	assert(refcount <= 1);
}

void Refcountable::Refer_Renamed()
{
	ScopedLock lock(&reflock);
	refcount++;
}

void Refcountable::Unref_Renamed()
{
	assert(!being_deleted);
	kthread_mutex_lock(&reflock);
	assert(refcount);
	bool deleteme = !refcount || !--refcount;
	kthread_mutex_unlock(&reflock);
	if ( deleteme )
	{
		being_deleted = true;
		delete this;
	}
}

} // namespace Sortix
