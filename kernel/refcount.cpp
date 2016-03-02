/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * refcount.cpp
 * A class that implements reference counting.
 */

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
