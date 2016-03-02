/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * mtable.cpp
 * Class to keep track of mount points.
 */

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

bool MountTable::AddMount(ino_t ino, dev_t dev, Ref<Inode> inode, bool fsbind)
{
	ScopedLock lock(&mtablelock);
	return AddMountUnlocked(ino, dev, inode, fsbind);
}

bool MountTable::AddMountUnlocked(ino_t ino, dev_t dev, Ref<Inode> inode, bool fsbind)
{
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
	mp->fsbind = fsbind;
	return true;
}

} // namespace Sortix
