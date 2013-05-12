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

    identity.cpp
    System calls for managing user and group identities.

*******************************************************************************/

#include <sortix/syscallnum.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/process.h>

#include "identity.h"

namespace Sortix {
namespace Identity {

static uid_t sys_getuid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->uid;
}

static int sys_setuid(uid_t uid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->uid = uid, 0;
}

static gid_t sys_getgid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->gid;
}

static int sys_setgid(gid_t gid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->gid = gid, 0;
}

static uid_t sys_geteuid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->euid;
}

static int sys_seteuid(uid_t euid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->euid = euid, 0;
}

static gid_t sys_getegid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->egid;
}

static int sys_setegid(gid_t egid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->egid = egid, 0;
}

void Init()
{
	Syscall::Register(SYSCALL_GETUID, (void*) sys_getuid);
	Syscall::Register(SYSCALL_GETGID, (void*) sys_getgid);
	Syscall::Register(SYSCALL_SETUID, (void*) sys_setuid);
	Syscall::Register(SYSCALL_SETGID, (void*) sys_setgid);
	Syscall::Register(SYSCALL_GETEUID, (void*) sys_geteuid);
	Syscall::Register(SYSCALL_GETEGID, (void*) sys_getegid);
	Syscall::Register(SYSCALL_SETEUID, (void*) sys_seteuid);
	Syscall::Register(SYSCALL_SETEGID, (void*) sys_setegid);
}

} // namespace Identity
} // namespace Sortix
