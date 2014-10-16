/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    resource.cpp
    Resource limits and operations.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <limits.h>

#include <sortix/resource.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/ptable.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

static int GetProcessPriority(pid_t who)
{
	if ( who < 0 )
		return errno = EINVAL, -1;
	// TODO: If who isn't the current process, then it could self-destruct at
	//       any time while we use it; there is no safe way to do this yet.
	Process* process = who ? CurrentProcess()->GetPTable()->Get(who) : CurrentProcess();
	if ( !process )
		return errno = ESRCH, -1;
	ScopedLock lock(&process->nicelock);
	return process->nice;
}

static int SetProcessPriority(pid_t who, int prio)
{
	if ( who < 0 )
		return errno = EINVAL, -1;
	// TODO: If who isn't the current process, then it could self-destruct at
	//       any time while we use it; there is no safe way to do this yet.
	Process* process = who ? CurrentProcess()->GetPTable()->Get(who) : CurrentProcess();
	if ( !process )
		return errno = ESRCH, -1;
	ScopedLock lock(&process->nicelock);
	process->nice = prio;
	return 0;
}

static Process* CurrentProcessGroup()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->groupchildlock);
	// TODO: The process group can change when this call returns, additionally
	//       the current process leader could self-destruct.
	return process->group;
}

static int GetProcessGroupPriority(pid_t who)
{
	if ( who < 0 )
		return errno = EINVAL, -1;
	// TODO: If who isn't the current process, then it could self-destruct at
	//       any time while we use it; there is no safe way to do this yet.
	Process* group = who ? CurrentProcess()->GetPTable()->Get(who) : CurrentProcessGroup();
	if ( !group )
		return errno = ESRCH, -1;
	int lowest = INT_MAX;
	ScopedLock group_parent_lock(&group->groupparentlock);
	for ( Process* process = group->groupfirst; process; process = process->groupnext )
	{
		ScopedLock lock(&process->nicelock);
		if ( process->nice < lowest )
			lowest = process->nice;
	}
	return lowest;
}

static int SetProcessGroupPriority(pid_t who, int prio)
{
	if ( who < 0 )
		return errno = EINVAL, -1;
	// TODO: If who isn't the current process, then it could self-destruct at
	//       any time while we use it; there is no safe way to do this yet.
	Process* group = who ? CurrentProcess()->GetPTable()->Get(who) : CurrentProcessGroup();
	if ( !group )
		return errno = ESRCH, -1;
	ScopedLock group_parent_lock(&group->groupparentlock);
	for ( Process* process = group->groupfirst; process; process = process->groupnext )
	{
		ScopedLock lock(&process->nicelock);
		process->nice = prio;
	}
	return 0;
}

static int GetUserPriority(uid_t /*who*/)
{
	// TODO: There is currently no easy way to iterate all processes without
	//       dire race conditions being possible.
	return errno = ENOSYS, -1;
}

static int SetUserPriority(uid_t /*who*/, int /*prio*/)
{
	// TODO: There is currently no easy way to iterate all processes without
	//       dire race conditions being possible.
	return errno = ENOSYS, -1;
}

int sys_getpriority(int which, id_t who)
{
	switch ( which )
	{
	case PRIO_PROCESS: return GetProcessPriority(who);
	case PRIO_PGRP: return GetProcessGroupPriority(who);
	case PRIO_USER: return GetUserPriority(who);
	default: return errno = EINVAL, -1;
	}
}

int sys_setpriority(int which, id_t who, int prio)
{
	switch ( which )
	{
	case PRIO_PROCESS: return SetProcessPriority(who, prio);
	case PRIO_PGRP: return SetProcessGroupPriority(who, prio);
	case PRIO_USER: return SetUserPriority(who, prio);
	default: return errno = EINVAL, -1;
	}
}

int sys_prlimit(pid_t pid,
                int resource,
                const struct rlimit* user_new_limit,
                struct rlimit* user_old_limit)
{
	if ( pid < 0 )
		return errno = EINVAL, -1;
	if ( resource < 0 || RLIMIT_NUM_DECLARED <= resource )
		return errno = EINVAL, -1;
	// TODO: If pid isn't the current process, then it could self-destruct at
	//       any time while we use it; there is no safe way to do this yet.
	Process* process = pid ? CurrentProcess()->GetPTable()->Get(pid) : CurrentProcess();
	if ( !process )
		return errno = ESRCH, -1;
	ScopedLock lock(&process->resource_limits_lock);
	struct rlimit* limit = &process->resource_limits[resource];
	if ( user_old_limit )
	{
		if ( !CopyToUser(user_old_limit, limit, sizeof(struct rlimit)) )
			return -1;
	}
	if ( user_new_limit )
	{
		struct rlimit new_limit;
		if ( !CopyFromUser(&new_limit, user_new_limit, sizeof(struct rlimit)) )
			return -1;
		if ( new_limit.rlim_max < new_limit.rlim_cur )
			return errno = EINVAL, -1;
		*limit = new_limit;
	}
	return 0;
}

} // namespace Sortix
