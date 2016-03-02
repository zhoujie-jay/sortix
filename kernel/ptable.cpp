/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * ptable.cpp
 * Process table.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/ptable.h>
#include <sortix/kernel/refcount.h>

// TODO: Process memory ownership needs to be reference counted.
// TODO: This implementation is rather run-time inefficient.
// TODO: The Free method potentially leaks memory as the ptable is never shrunk.
// TODO: The next_pid counter could potentially overflow.

namespace Sortix {

ProcessTable::ProcessTable()
{
	ptablelock = KTHREAD_MUTEX_INITIALIZER;
	next_pid = 0;
	entries = NULL;
	entries_used = 0;
	entries_length = 0;
}

ProcessTable::~ProcessTable()
{
	delete[] entries;
}

Process* ProcessTable::Get(pid_t pid)
{
	ScopedLock lock(&ptablelock);
	for ( size_t i = 0; i < entries_used; i++ )
		if ( entries[i].pid == pid )
			return entries[i].process;
	return errno = ESRCH, (Process*) NULL;
}

pid_t ProcessTable::Allocate(Process* process)
{
	ScopedLock lock(&ptablelock);
	if ( entries_used == entries_length )
	{
		size_t new_length = entries_length ? 2 * entries_length : 64;
		struct ptable_entry* new_entries = new struct ptable_entry[new_length];
		if ( !new_entries )
			return -1;
		if ( entries )
		{
			size_t old_size = sizeof(struct ptable_entry) * entries_length;
			memcpy(new_entries, entries, old_size);
			delete[] entries;
		}
		entries = new_entries;
		entries_length = new_length;
	}

	struct ptable_entry* entry = &entries[entries_used++];
	entry->process = process;
	return entry->pid = next_pid++;
}

void ProcessTable::Free(pid_t pid)
{
	ScopedLock lock(&ptablelock);
	for ( size_t i = 0; i < entries_used; i++ )
	{
		if ( entries[i].pid != pid )
			continue;
		entries[i] = entries[--entries_used];
		return;
	}
	assert(false);
}

pid_t ProcessTable::Prev(pid_t pid)
{
	ScopedLock lock(&ptablelock);
	pid_t result = -1;
	for ( size_t i = 0; i < entries_used; i++ )
	{
		if ( pid <= entries[i].process->pid )
			continue;
		if ( result == -1 || result < entries[i].process->pid )
			result = entries[i].process->pid;
	}
	return result;
}

pid_t ProcessTable::Next(pid_t pid)
{
	ScopedLock lock(&ptablelock);
	pid_t result = -1;
	for ( size_t i = 0; i < entries_used; i++ )
	{
		if ( entries[i].process->pid <= pid )
			continue;
		if ( result == -1 || entries[i].process->pid < result )
			result = entries[i].process->pid;
	}
	return result;
}

} // namespace Sortix
