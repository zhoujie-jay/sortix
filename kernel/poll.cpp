/*
 * Copyright (c) 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * poll.cpp
 * Interface for waiting on file descriptor events.
 */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <timespec.h>

#include <sortix/clock.h>
#include <sortix/poll.h>
#include <sortix/sigset.h>
#include <sortix/timespec.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/timer.h>

#include "poll.h"

namespace Sortix {

PollChannel::PollChannel()
{
	first = NULL;
	last = NULL;
	channel_lock = KTHREAD_MUTEX_INITIALIZER;
	no_pending_cond = KTHREAD_COND_INITIALIZER;
}

PollChannel::~PollChannel()
{
	ScopedLock lock(&channel_lock);
	// TODO: Is this the correct error to signal with?
	SignalUnlocked(POLLHUP);
	// Note: We can't stop early in case of a signal, because that would mean
	// other threads are still using our data, and since this is the destructor,
	// leaving early _will_ cause data corruption. Luckily, this loop will
	// terminate because everyone is now woken up and will cancel, which is what
	// we wait for to finish. No new requests can come, since we are the
	// destructor - whoever owns this object is no longer using it.
	while ( first )
		kthread_cond_wait(&no_pending_cond, &channel_lock);
}

void PollChannel::Signal(short events)
{
	ScopedLock lock(&channel_lock);
	SignalUnlocked(events);
}

void PollChannel::SignalUnlocked(short events)
{
	for ( PollNode* node = first; node; node = node->next )
	{
		PollNode* target = node->master;
		if ( target->revents |= events & (target->events | POLL__ONLY_REVENTS) )
		{
			ScopedLock target_lock(target->wake_mutex);
			if ( !*target->woken )
			{
				*target->woken = true;
				kthread_cond_signal(target->wake_cond);
			}
		}
	}
}

void PollChannel::Register(PollNode* node)
{
	ScopedLock lock(&channel_lock);
	assert(!node->channel);
	node->channel = this;
	if ( !first )
		first = last = node,
		node->next = node->prev = NULL;
	else
		node->next = NULL,
		node->prev = last,
		last->next = node,
		last = node;
}

void PollChannel::Unregister(PollNode* node)
{
	ScopedLock lock(&channel_lock);
	node->channel = NULL;
	if ( node->prev )
		node->prev->next = node->next;
	else
		first = node->next;
	if ( node->next )
		node->next->prev = node->prev;
	else
		last = node->prev;
	if ( !first )
		kthread_cond_signal(&no_pending_cond);
}

void PollNode::Cancel()
{
	if ( channel )
		channel->Unregister(this);
	if ( slave )
		slave->Cancel();
}

PollNode* PollNode::CreateSlave()
{
	PollNode* new_slave = new PollNode();
	if ( !new_slave )
		return NULL;
	new_slave->wake_mutex = wake_mutex;
	new_slave->wake_cond = wake_cond;
	new_slave->events = events;
	new_slave->revents = revents;
	new_slave->woken = woken;
	new_slave->master = master;
	new_slave->slave = slave;
	return slave = new_slave;
}

static struct pollfd* CopyFdsFromUser(struct pollfd* user_fds, size_t nfds)
{
	size_t size = sizeof(struct pollfd) * nfds;
	struct pollfd* fds = new struct pollfd[nfds];
	if ( !fds )
		return NULL;
	if ( !CopyFromUser(fds, user_fds, size) )
	{
		delete[] fds;
		return NULL;
	}
	return fds;
}

static bool CopyFdsToUser(struct pollfd* user_fds,
                          const struct pollfd* kernel_fds, size_t nfds)
{
	size_t size = sizeof(struct pollfd) * nfds;
	return CopyToUser(user_fds, kernel_fds, size);
}

static bool FetchTimespec(struct timespec* dest, const struct timespec* user)
{
	if ( !user )
		dest->tv_sec = -1,
		dest->tv_nsec = 0;
	else if ( !CopyFromUser(dest, user, sizeof(*dest)) )
		return false;
	return true;
}

struct poll_timeout
{
	kthread_mutex_t* wake_mutex;
	kthread_cond_t* wake_cond;
	bool* woken;
};

static void poll_timeout_callback(Clock*, Timer*, void* ctx)
{
	struct poll_timeout* pts = (struct poll_timeout*) ctx;
	ScopedLock lock(pts->wake_mutex);
	*pts->woken = true;
	kthread_cond_signal(pts->wake_cond);
}

int sys_ppoll(struct pollfd* user_fds, size_t nfds,
              const struct timespec* user_timeout_ts,
              const sigset_t* user_sigmask)
{
	ioctx_t ctx; SetupKernelIOCtx(&ctx);

	struct timespec timeout_ts;
	if ( !FetchTimespec(&timeout_ts, user_timeout_ts) )
		return -1;

	if ( user_sigmask )
		return errno = ENOSYS, -1;

	struct pollfd* fds = CopyFdsFromUser(user_fds, nfds);
	if ( !fds ) { return -1; }

	PollNode* nodes = new PollNode[nfds];
	if ( !nodes ) { delete[] fds; return -1; }

	Process* process = CurrentProcess();

	kthread_mutex_t wakeup_mutex = KTHREAD_MUTEX_INITIALIZER;
	kthread_cond_t wakeup_cond = KTHREAD_COND_INITIALIZER;

	kthread_mutex_lock(&wakeup_mutex);

	int ret = -1;
	bool self_woken = false;
	bool remote_woken = false;
	bool unexpected_error = false;

	Timer timer;
	struct poll_timeout pts;
	if ( timespec_le(timespec_make(0, 1), timeout_ts) )
	{
		timer.Attach(Time::GetClock(CLOCK_MONOTONIC));
		struct itimerspec its;
		its.it_interval = timespec_nul();
		its.it_value = timeout_ts;
		pts.wake_mutex = &wakeup_mutex;
		pts.wake_cond = &wakeup_cond;
		pts.woken = &remote_woken;
		timer.Set(&its, NULL, 0, poll_timeout_callback, &pts);
	}

	size_t reqs;
	for ( reqs = 0; !unexpected_error && reqs < nfds; )
	{
		PollNode* node = nodes + reqs;
		if ( fds[reqs].fd < 0 )
		{
			fds[reqs].revents = POLLNVAL;
			// TODO: Should we set POLLNVAL in node->revents too? Should this
			// system call ignore this error and keep polling, or return to
			// user-space immediately? What if conditions are already true on
			// some of the file descriptors (those we have processed so far?)?
			node->revents = 0;
			reqs++;
			continue;
		}
		Ref<Descriptor> desc = process->GetDescriptor(fds[reqs].fd);
		if ( !desc ) { self_woken = unexpected_error = true; break; }
		node->events = fds[reqs].events | POLL__ONLY_REVENTS;
		node->revents = 0;
		node->wake_mutex = &wakeup_mutex;
		node->wake_cond = &wakeup_cond;
		node->woken = &remote_woken;
		reqs++;
		// TODO: How should errors be handled?
		if ( desc->poll(&ctx, node) == 0 )
			self_woken = true;
		else if ( errno == EAGAIN )
			errno = 0;
		else
			unexpected_error = self_woken = true;
	}

	if ( timeout_ts.tv_sec == 0 && timeout_ts.tv_nsec == 0 )
		self_woken = true;

	while ( !(self_woken || remote_woken) )
	{
		if ( !kthread_cond_wait_signal(&wakeup_cond, &wakeup_mutex) )
			errno = -EINTR,
			self_woken = true;
	}

	kthread_mutex_unlock(&wakeup_mutex);

	for ( size_t i = 0; i < reqs; i++ )
		if ( 0 <= fds[i].fd )
			nodes[i].Cancel();

	if ( timespec_le(timespec_make(0, 1), timeout_ts) )
	{
		timer.Cancel();
		timer.Detach();
	}

	if ( !unexpected_error )
	{
		int num_events = 0;
		for ( size_t i = 0; i < reqs; i++ )
		{
			if ( fds[i].fd < -1 )
				continue;
			if ( (fds[i].revents = nodes[i].revents) )
				num_events++;
		}

		if ( CopyFdsToUser(user_fds, fds, nfds) )
			ret = num_events;
	}

	delete[] nodes;
	delete[] fds;
	return ret;
}

} // namespace Sortix
