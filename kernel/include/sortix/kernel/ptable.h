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

    sortix/kernel/ptable.h
    Process table.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEl_PTABLE_H
#define INCLUDE_SORTIX_KERNEl_PTABLE_H

#include <sys/types.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

class Process;

struct ptable_entry
{
	pid_t pid;
	Process* process;
};

class ProcessTable : public Refcountable
{
public:
	ProcessTable();
	virtual ~ProcessTable();
	Process* Get(pid_t pid);
	pid_t Allocate(Process* process);
	void Free(pid_t pid);

private:
	kthread_mutex_t ptablelock;
	pid_t next_pid;
	struct ptable_entry* entries;
	size_t entries_used;
	size_t entries_length;

};

} // namespace Sortix

#endif
