/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * psctl.cpp
 * Process control interface.
 */

#include <sys/types.h>

#include <errno.h>
#include <string.h>

#include <sortix/psctl.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/ptable.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

int sys_psctl(pid_t pid, int request, void* ptr)
{
	Ref<ProcessTable> ptable = CurrentProcess()->GetPTable();
	if ( request == PSCTL_PREV_PID )
	{
		struct psctl_prev_pid resp;
		memset(&resp, 0, sizeof(resp));
		resp.prev_pid = ptable->Prev(pid);
		return CopyToUser(ptr, &resp, sizeof(resp)) ? 0 : -1;
	}
	else if ( request == PSCTL_NEXT_PID )
	{
		struct psctl_next_pid resp;
		memset(&resp, 0, sizeof(resp));
		resp.next_pid = ptable->Next(pid);
		return CopyToUser(ptr, &resp, sizeof(resp)) ? 0 : -1;
	}
	// TODO: Scoped lock that prevents zombies from terminating.
	Process* process = ptable->Get(pid);
	if ( !process )
		return errno = ESRCH, -1;
	if ( request == PSCTL_STAT )
	{
		struct psctl_stat psst;
		memset(&psst, 0, sizeof(psst));
		psst.pid = pid;
		kthread_mutex_lock(&process->parentlock);
		if ( process->parent )
		{
			Process* parent = process->parent;
			psst.ppid = parent->pid;
			kthread_mutex_unlock(&process->parentlock);
			// TODO: Is there a risk of getting a new parent here?
			kthread_mutex_lock(&parent->childlock);
			psst.ppid_prev = process->prevsibling ? process->prevsibling->pid : -1;
			psst.ppid_next = process->nextsibling ? process->nextsibling->pid : -1;
			kthread_mutex_unlock(&parent->childlock);
		}
		else
		{
			kthread_mutex_unlock(&process->parentlock);
			psst.ppid = -1;
			psst.ppid_prev = -1;
			psst.ppid_next = -1;
		}
		kthread_mutex_lock(&process->childlock);
		psst.ppid_first = process->firstchild ? process->firstchild->pid : -1;
		kthread_mutex_unlock(&process->childlock);
		kthread_mutex_lock(&process->groupparentlock);
		if ( process->group )
		{
			Process* group = process->group;
			psst.pgid = group->pid;
			kthread_mutex_unlock(&process->groupparentlock);
			// TODO: Is there a risk of getting a new group here?
			kthread_mutex_lock(&group->groupchildlock);
			psst.pgid_prev = process->groupprev ? process->groupprev->pid : -1;
			psst.pgid_next = process->groupnext ? process->groupnext->pid : -1;
			kthread_mutex_unlock(&group->groupchildlock);
		}
		else
		{
			kthread_mutex_unlock(&process->groupparentlock);
			psst.pgid = -1;
			psst.pgid_prev = -1;
			psst.pgid_next = -1;
		}
		kthread_mutex_lock(&process->groupchildlock);
		psst.pgid_first = process->groupfirst ? process->groupfirst->pid : -1;
		kthread_mutex_unlock(&process->groupchildlock);
		// TODO: Implement sessions.
		psst.sid = 1;
		psst.sid_prev = ptable->Prev(pid);
		psst.sid_next = ptable->Next(pid);
		psst.sid_first = pid == 1 ? 1 : -1;
		// TODO: Implement init groupings.
		psst.init = 1;
		psst.init_prev = ptable->Prev(pid);
		psst.init_next = ptable->Next(pid);
		psst.init_first = pid == 1 ? 1 : -1;
		kthread_mutex_lock(&process->threadlock);
		psst.status = process->exit_code;
		kthread_mutex_unlock(&process->threadlock);
		kthread_mutex_lock(&process->nicelock);
		psst.nice = process->nice;
		kthread_mutex_unlock(&process->nicelock);
		// Note: It is safe to access the clocks in this manner as each of them
		//       are locked by disabling interrupts. This is perhaps not
		//       SMP-ready, but it will do for now.
		Interrupt::Disable();
		psst.tmns.tmns_utime = process->execute_clock.current_time;
		psst.tmns.tmns_stime = process->system_clock.current_time;
		psst.tmns.tmns_cutime = process->child_execute_clock.current_time;
		psst.tmns.tmns_cstime = process->child_system_clock.current_time;
		Interrupt::Enable();
		return CopyToUser(ptr, &psst, sizeof(psst)) ? 0 : -1;
	}
	else if ( request == PSCTL_PROGRAM_PATH )
	{
		struct psctl_program_path ctl;
		if ( !CopyFromUser(&ctl, ptr, sizeof(ctl)) )
			return -1;
		// TODO: program_image_path is not properly protected at this time.
		const char* path = process->program_image_path;
		if ( !path )
			path = "";
		size_t size = strlen(path) + 1;
		struct psctl_program_path resp = ctl;
		resp.size = size;
		if ( !CopyToUser(ptr, &resp, sizeof(resp)) )
			return -1;
		if ( ctl.buffer )
		{
			if ( ctl.size < size )
				return errno = ERANGE, -1;
			if ( !CopyToUser(ctl.buffer, path, size) )
				return -1;
		}
		return 0;
	}
	return errno = EINVAL, -1;
}

} // namespace Sortix
