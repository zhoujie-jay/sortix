/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    ptable.cpp
    Process table.

*******************************************************************************/

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
