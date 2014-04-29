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

    sortix/kernel/poll.h
    Kernel declarations for event polling.

*******************************************************************************/

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
