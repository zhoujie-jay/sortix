/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * dtable.cpp
 * Table of file descriptors.
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include <sortix/fcntl.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

DescriptorTable::DescriptorTable()
{
	dtablelock = KTHREAD_MUTEX_INITIALIZER;
	entries = NULL;
	numentries = 0;
	first_not_taken = 0;
}

DescriptorTable::~DescriptorTable()
{
	Reset();
}

bool DescriptorTable::IsGoodEntry(int i)
{
	return 0 <= i && i < numentries && entries[i].desc;
}

Ref<DescriptorTable> DescriptorTable::Fork()
{
	ScopedLock lock(&dtablelock);
	Ref<DescriptorTable> ret(new DescriptorTable);
	if ( !ret )
		return Ref<DescriptorTable>(NULL);
	ret->entries = new dtableent_t[numentries];
	if ( !ret->entries )
		return Ref<DescriptorTable>(NULL);
	ret->first_not_taken = 0;
	ret->numentries = numentries;
	for ( int i = 0; i < numentries; i++ )
	{
		if ( !entries[i].desc || entries[i].flags & FD_CLOFORK )
		{
			//ret->entries[i].desc = NULL; // Constructor did this already.
			ret->entries[i].flags = 0;
			continue;
		}
		ret->entries[i] = entries[i];
		if ( ret->first_not_taken == i )
			ret->first_not_taken = i + 1;
	}
	return ret;
}

Ref<Descriptor> DescriptorTable::Get(int index)
{
	ScopedLock lock(&dtablelock);
	if ( !IsGoodEntry(index) )
		return errno = EBADF, Ref<Descriptor>(NULL);
	return entries[index].desc;
}

bool DescriptorTable::Enlargen(int atleast)
{
	if ( numentries == INT_MAX )
		return errno = EMFILE, false; // Cannot enlargen any more.
	// TODO: Modern overflow checks.
	int newnumentries = INT_MAX - numentries < numentries ?
	                    INT_MAX :
                        numentries ? 2 * numentries : 8;
	if ( newnumentries < atleast )
		newnumentries = atleast;
	dtableent_t* newentries = new dtableent_t[newnumentries];
	if ( !newentries )
		return false;
	for ( int i = 0; i < numentries; i++ )
	{
		newentries[i] = entries[i];
		entries[i].desc.Reset();
	}
	for ( int i = numentries; i < newnumentries; i++ )
	{
		//newentries[i].desc = NULL; // Constructor did this already.
		newentries[i].flags = 0;
	}
	delete[] entries;
	entries = newentries;
	numentries = newnumentries;
	return true;
}

int DescriptorTable::AllocateInternal(Ref<Descriptor> desc,
                                      int flags,
                                      int min_index)
{
	// dtablelock is held.
	if ( flags & ~__FD_ALLOWED_FLAGS )
		return errno = EINVAL, -1;
	if ( min_index < 0 )
		return errno = EINVAL, -1;
	if ( min_index < first_not_taken )
		min_index = first_not_taken;
	for ( int i = min_index; i < numentries; i++ )
	{
		if ( entries[i].desc )
			continue;
		entries[i].desc = desc;
		entries[i].flags = flags;
		if ( first_not_taken == i )
			first_not_taken = i + 1;
		return i;
	}
	first_not_taken = numentries;
	int oldnumentries = numentries;
	if ( !Enlargen(min_index) )
		return -1;
	int i = oldnumentries;
	entries[i].desc = desc;
	entries[i].flags = flags;
	if ( first_not_taken == i )
		first_not_taken = i + 1;
	return i;
}

int DescriptorTable::Allocate(Ref<Descriptor> desc, int flags, int min_index)
{
	ScopedLock lock(&dtablelock);
	return AllocateInternal(desc, flags, min_index);
}

int DescriptorTable::Allocate(int src_index, int flags, int min_index)
{
	ScopedLock lock(&dtablelock);
	if ( !IsGoodEntry(src_index) )
		return errno = EBADF, -1;
	return AllocateInternal(entries[src_index].desc, flags, min_index);
}

int DescriptorTable::Copy(int from, int to, int flags)
{
	if ( flags & ~__FD_ALLOWED_FLAGS )
		return errno = EINVAL, -1;
	ScopedLock lock(&dtablelock);
	if ( from < 0 || to < 0 )
		return errno = EINVAL, -1;
	if ( !(from < numentries) )
		return errno = EBADF, -1;
	if ( !entries[from].desc )
		return errno = EBADF, -1;
	if ( from == to )
		return errno = EINVAL, -1;
	while ( !(to < numentries) )
	{
		if ( to == INT_MAX )
			return errno = EBADF, -1;
		if ( !Enlargen(to + 1) )
			return -1;
	}
	if ( entries[to].desc != entries[from].desc )
	{
		if ( entries[to].desc )
		{
			// TODO: Should this be synced or otherwise properly closed?
		}
		entries[to].desc = entries[from].desc;
	}
	entries[to].flags = flags;
	if ( first_not_taken == to )
		first_not_taken = to + 1;
	return to;
}

Ref<Descriptor> DescriptorTable::FreeKeepInternal(int index)
{
	if ( !IsGoodEntry(index) )
		return errno = EBADF, Ref<Descriptor>(NULL);
	Ref<Descriptor> ret = entries[index].desc;
	entries[index].desc.Reset();
	entries[index].flags = 0;
	if ( index < first_not_taken )
		first_not_taken = index;
	return ret;
}

Ref<Descriptor> DescriptorTable::FreeKeep(int index)
{
	ScopedLock lock(&dtablelock);
	return FreeKeepInternal(index);
}

void DescriptorTable::Free(int index)
{
	FreeKeep(index);
}

void DescriptorTable::OnExecute()
{
	ScopedLock lock(&dtablelock);
	for ( int i = 0; i < numentries; i++ )
	{
		if ( !entries[i].desc )
			continue;
		if ( !(entries[i].flags & FD_CLOEXEC) )
			continue;
		entries[i].desc.Reset();
		if ( i < first_not_taken )
			first_not_taken = i;
	}
}

void DescriptorTable::Reset()
{
	ScopedLock lock(&dtablelock);
	for ( int i = 0; i < numentries; i++ )
		if ( entries[i].desc )
			entries[i].desc.Reset();
	numentries = 0;
	delete[] entries;
	entries = NULL;
	first_not_taken = 0;
}

bool DescriptorTable::SetFlags(int index, int flags)
{
	if ( flags & ~__FD_ALLOWED_FLAGS )
		return errno = EINVAL, false;
	ScopedLock lock(&dtablelock);
	if ( !IsGoodEntry(index) )
		return errno = EBADF, false;
	entries[index].flags = flags;
	return true;
}

int DescriptorTable::GetFlags(int index)
{
	ScopedLock lock(&dtablelock);
	if ( !IsGoodEntry(index) )
		return errno = EBADF, -1;
	return entries[index].flags;
}

int DescriptorTable::Previous(int index)
{
	ScopedLock lock(&dtablelock);
	if ( index < 0 )
		index = numentries;
	do index--;
	while ( 0 <= index && !IsGoodEntry(index) );
	if ( index < 0 )
		return errno = EBADF, -1;
	return index;
}

int DescriptorTable::Next(int index)
{
	ScopedLock lock(&dtablelock);
	if ( index < 0 )
		index = -1;
	if ( numentries <= index )
		return errno = EBADF, -1;
	do index++;
	while ( index < numentries && !IsGoodEntry(index) );
	if ( numentries <= index )
		return errno = EBADF, -1;
	return index;
}

int DescriptorTable::CloseFrom(int index)
{
	if ( index < 0 )
		return errno = EBADF, -1;
	ScopedLock lock(&dtablelock);
	bool any = false;
	for ( ; index < numentries; index++ )
	{
		if ( !IsGoodEntry(index) )
			continue;
		FreeKeepInternal(index);
		any = true;
	}
	return any ? 0 : (errno = EBADF, -1);
}

} // namespace Sortix
