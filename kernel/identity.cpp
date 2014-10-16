/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

#include <sys/types.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

uid_t sys_getuid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->uid;
}

int sys_setuid(uid_t uid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->uid = uid, 0;
}

gid_t sys_getgid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->gid;
}

int sys_setgid(gid_t gid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->gid = gid, 0;
}

uid_t sys_geteuid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->euid;
}

int sys_seteuid(uid_t euid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->euid = euid, 0;
}

gid_t sys_getegid()
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->egid;
}

int sys_setegid(gid_t egid)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	return process->egid = egid, 0;
}

} // namespace Sortix
