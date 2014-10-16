/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    poll.cpp
    Interface for waiting on file descriptor events.

*******************************************************************************/

#include <sys/types.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <sortix/poll.h>
#include <sortix/sigset.h>
#include <sortix/timespec.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/syscall.h>

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

static struct pollfd* CopyFdsFromUser(struct pollfd* user_fds, nfds_t nfds)
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
                          const struct pollfd* kernel_fds, nfds_t nfds)
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

int sys_ppoll(struct pollfd* user_fds, nfds_t nfds,
              const struct timespec* user_timeout_ts,
              const sigset_t* user_sigmask)
{
	ioctx_t ctx; SetupKernelIOCtx(&ctx);

	struct timespec timeout_ts;
	if ( !FetchTimespec(&timeout_ts, user_timeout_ts) )
		return -1;

	if ( 0 < timeout_ts.tv_sec || timeout_ts.tv_nsec || user_sigmask )
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
	volatile bool remote_woken = false;
	bool unexpected_error = false;

	nfds_t reqs = nfds;
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
		node->woken = (bool*) &remote_woken;
		reqs++;
		// TODO: How should errors be handled?
		if ( desc->poll(&ctx, node) == 0 )
			self_woken = true;
		else if ( errno != EAGAIN )
			unexpected_error = self_woken = true;
	}

	if ( timeout_ts.tv_sec < 0 )
		self_woken = true;

	while ( !(self_woken || remote_woken) )
	{
		if ( !kthread_cond_wait_signal(&wakeup_cond, &wakeup_mutex) )
			errno = -EINTR,
			self_woken = true;
	}

	kthread_mutex_unlock(&wakeup_mutex);

	for ( nfds_t i = 0; i < reqs; i++ )
		if ( 0 <= fds[i].fd )
			nodes[i].Cancel();

	if ( !unexpected_error )
	{
		int num_events = 0;
		for ( nfds_t i = 0; i < reqs; i++ )
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
