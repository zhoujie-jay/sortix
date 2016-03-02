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
 * sortix/kernel/ptable.h
 * Process table.
 */

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
	pid_t Prev(pid_t pid);
	pid_t Next(pid_t pid);

private:
	kthread_mutex_t ptablelock;
	pid_t next_pid;
	struct ptable_entry* entries;
	size_t entries_used;
	size_t entries_length;

};

} // namespace Sortix

#endif
