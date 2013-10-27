/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    mtable.cpp
    Class to keep track of mount points.

*******************************************************************************/

#include <string.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

MountTable::MountTable()
{
	mtablelock = KTHREAD_MUTEX_INITIALIZER;
	mounts = NULL;
	nummounts = 0;
	mountsalloced = 0;
}

MountTable::~MountTable()
{
	delete[] mounts;
}

Ref<MountTable> MountTable::Fork()
{
	ScopedLock lock(&mtablelock);
	Ref<MountTable> clone(new MountTable);
	if ( !clone )
		return Ref<MountTable>(NULL);
	clone->mounts = new mountpoint_t[nummounts];
	if ( !clone->mounts )
		return Ref<MountTable>(NULL);
	clone->nummounts = nummounts;
	clone->mountsalloced = nummounts;
	for ( size_t i = 0; i < nummounts; i++ )
		clone->mounts[i] = mounts[i];
	return clone;
}

bool MountTable::AddMount(ino_t ino, dev_t dev, Ref<Inode> inode)
{
	ScopedLock lock(&mtablelock);
	if ( nummounts == mountsalloced )
	{
		size_t newalloced = mountsalloced ? 2UL * mountsalloced : 4UL;
		mountpoint_t* newmounts = new mountpoint_t[newalloced];
		if ( !newmounts )
			return false;
		for ( size_t i = 0; i < nummounts; i++ )
			newmounts[i] = mounts[i];
		delete[] mounts;
		mounts = newmounts;
		mountsalloced = newalloced;
	}
	mountpoint_t* mp = mounts + nummounts++;
	mp->ino = ino;
	mp->dev = dev;
	mp->inode = inode;
	return true;
}

} // namespace Sortix
