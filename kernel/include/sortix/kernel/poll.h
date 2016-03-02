/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/poll.h
 * Kernel declarations for event polling.
 */

#ifndef INCLUDE_SORTIX_KERNEL_POLL_H
#define INCLUDE_SORTIX_KERNEL_POLL_H

#include <sortix/kernel/kthread.h>

namespace Sortix {

class PollChannel;
class PollNode;

class PollChannel
{
public:
	PollChannel();
	~PollChannel();
	void Signal(short events);
	void Register(PollNode* node);
	void Unregister(PollNode* node);

private:
	void SignalUnlocked(short events);

private:
	struct PollNode* first;
	struct PollNode* last;
	kthread_mutex_t channel_lock;
	kthread_cond_t no_pending_cond;

};

class PollNode
{
	friend class PollChannel;

public:
	PollNode() { next = NULL; prev = NULL; channel = NULL; master = this; slave = NULL; }
	~PollNode() { delete slave; }

private:
	PollNode* next;
	PollNode* prev;

public:
	PollChannel* channel;
	PollNode* master;
	PollNode* slave;

public:
	kthread_mutex_t* wake_mutex;
	kthread_cond_t* wake_cond;
	short events;
	short revents;
	bool* woken;

public:
	void Cancel();
	PollNode* CreateSlave();

};

} // namespace Sortix

#endif
