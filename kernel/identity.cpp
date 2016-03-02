/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * identity.cpp
 * System calls for managing user and group identities.
 */

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
