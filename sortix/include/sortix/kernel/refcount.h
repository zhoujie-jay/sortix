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

    sortix/kernel/refcount.h
    A class that implements reference counting.

*******************************************************************************/

#ifndef SORTIX_REFCOUNT_H
#define SORTIX_REFCOUNT_H

#include <sortix/kernel/kthread.h>

namespace Sortix {

class Refcounted
{
public:
	Refcounted();
	~Refcounted();

public:
	void Refer();
	void Unref();
	inline size_t Refcount() const { return refcount; }

private:
	kthread_mutex_t reflock;
	size_t refcount;

};

} // namespace Sortix

#endif
