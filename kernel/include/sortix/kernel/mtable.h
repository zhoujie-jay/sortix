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

    sortix/kernel/mtable.h
    Class to keep track of mount points.

*******************************************************************************/

#ifndef SORTIX_MTABLE_H
#define SORTIX_MTABLE_H

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

class Inode;

typedef struct
{
	Ref<Inode> inode;
	ino_t ino;
	dev_t dev;
} mountpoint_t;

class MountTable : public Refcountable
{
public:
	MountTable();
	~MountTable();
	Ref<MountTable> Fork();
	bool AddMount(ino_t ino, dev_t dev, Ref<Inode> inode);

public: // Consider these read-only.
	kthread_mutex_t mtablelock;
	mountpoint_t* mounts;
	size_t nummounts;

private:
	size_t mountsalloced;

};

} // namespace Sortix

#endif
