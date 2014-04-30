/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    pipe.cpp
    A device with a writing end and a reading end.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/fcntl.h>
#include <sortix/poll.h>
#include <sortix/signal.h>
#include <sortix/stat.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/pipe.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/vnode.h>

#include "pipe.h"

namespace Sortix {

class PipeChannel
{
public:
	PipeChannel(uint8_t* buffer, size_t buffersize);
	~PipeChannel();
	void CloseReading();
	void CloseWriting();
	void PerhapsShutdown();
	bool GetSIGPIPEDelivery();
	void SetSIGPIPEDelivery(bool deliver_sigpipe);
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	int read_poll(ioctx_t* ctx, PollNode* node);
	int write_poll(ioctx_t* ctx, PollNode* node);

private:
	short ReadPollEventStatus();
	short WritePollEventStatus();

private:
	PollChannel read_poll_channel;
	PollChannel write_poll_channel;
	kthread_mutex_t pipelock;
	kthread_cond_t readcond;
	kthread_cond_t writecond;
	uint8_t* buffer;
	size_t bufferoffset;
	size_t bufferused;
	size_t buffersize;
	bool anyreading;
	bool anywriting;
	bool is_sigpipe_enabled;

};

PipeChannel::PipeChannel(uint8_t* buffer, size_t buffersize)
{
	pipelock = KTHREAD_MUTEX_INITIALIZER;
	readcond = KTHREAD_COND_INITIALIZER;
	writecond = KTHREAD_COND_INITIALIZER;
	this->buffer = buffer;
	this->buffersize = buffersize;
	bufferoffset = bufferused = 0;
	anyreading = anywriting = true;
	is_sigpipe_enabled = true;
}

PipeChannel::~PipeChannel()
{
	delete[] buffer;
}

void PipeChannel::CloseReading()
{
	anyreading = false;
	kthread_cond_broadcast(&writecond);
	PerhapsShutdown();
}

void PipeChannel::CloseWriting()
{
	anywriting = false;
	kthread_cond_broadcast(&readcond);
	PerhapsShutdown();
}

void PipeChannel::PerhapsShutdown()
{
	kthread_mutex_lock(&pipelock);
	read_poll_channel.Signal(ReadPollEventStatus());
	write_poll_channel.Signal(WritePollEventStatus());
	bool deleteme = !anyreading & !anywriting;
	kthread_mutex_unlock(&pipelock);
	if ( deleteme )
		delete this;
}

ssize_t PipeChannel::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	ScopedLockSignal lock(&pipelock);
	if ( !lock.IsAcquired() ) { errno = EINTR; return -1; }
	while ( anywriting && !bufferused )
	{
		if ( ctx->dflags & O_NONBLOCK )
			return errno = EWOULDBLOCK, -1;
		if ( !kthread_cond_wait_signal(&readcond, &pipelock) )
		{
			errno = EINTR;
			return -1;
		}
	}
	if ( !bufferused && !anywriting ) { return 0; }
	if ( bufferused < count ) { count = bufferused; }
	size_t amount = count;
	size_t linear = buffersize - bufferoffset;
	if ( linear < amount ) { amount = linear; }
	assert(amount);
	ctx->copy_to_dest(buf, buffer + bufferoffset, amount);
	bufferoffset = (bufferoffset + amount) % buffersize;
	bufferused -= amount;
	kthread_cond_broadcast(&writecond);
	read_poll_channel.Signal(ReadPollEventStatus());
	write_poll_channel.Signal(WritePollEventStatus());
	return amount;
}

ssize_t PipeChannel::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	ScopedLockSignal lock(&pipelock);
	if ( !lock.IsAcquired() ) { errno = EINTR; return -1; }
	while ( anyreading && bufferused == buffersize )
	{
		if ( ctx->dflags & O_NONBLOCK )
			return errno = EWOULDBLOCK, -1;
		if ( !kthread_cond_wait_signal(&writecond, &pipelock) )
		{
			errno = EINTR;
			return -1;
		}
	}
	if ( !anyreading )
	{
		if ( is_sigpipe_enabled )
			CurrentThread()->DeliverSignal(SIGPIPE);
		return errno = EPIPE, -1;
	}
	if ( buffersize - bufferused < count ) { count = buffersize - bufferused; }
	size_t writeoffset = (bufferoffset + bufferused) % buffersize;
	size_t amount = count;
	size_t linear = buffersize - writeoffset;
	if ( linear < amount ) { amount = linear; }
	assert(amount);
	ctx->copy_from_src(buffer + writeoffset, buf, amount);
	bufferused += amount;
	kthread_cond_broadcast(&readcond);
	read_poll_channel.Signal(ReadPollEventStatus());
	write_poll_channel.Signal(WritePollEventStatus());
	return amount;
}

short PipeChannel::ReadPollEventStatus()
{
	short status = 0;
	if ( !anywriting && !bufferused )
		status |= POLLHUP;
	if ( bufferused )
		status |= POLLIN | POLLRDNORM;
	return status;
}

short PipeChannel::WritePollEventStatus()
{
	short status = 0;
	if ( !anyreading )
		status |= POLLERR;
	if ( anyreading && bufferused != buffersize )
		status |= POLLOUT | POLLWRNORM;
	return status;
}

int PipeChannel::read_poll(ioctx_t* /*ctx*/, PollNode* node)
{
	ScopedLockSignal lock(&pipelock);
	short ret_status = ReadPollEventStatus() & node->events;
	if ( ret_status )
		return node->master->revents |= ret_status, 0;
	read_poll_channel.Register(node);
	return errno = EAGAIN, -1;
}

int PipeChannel::write_poll(ioctx_t* /*ctx*/, PollNode* node)
{
	ScopedLockSignal lock(&pipelock);
	short ret_status = WritePollEventStatus() & node->events;
	if ( ret_status )
		return node->master->revents |= ret_status, 0;
	write_poll_channel.Register(node);
	return errno = EAGAIN, -1;
}

bool PipeChannel::GetSIGPIPEDelivery()
{
	ScopedLockSignal lock(&pipelock);
	return is_sigpipe_enabled;
}

void PipeChannel::SetSIGPIPEDelivery(bool deliver_sigpipe)
{
	ScopedLockSignal lock(&pipelock);
	is_sigpipe_enabled = deliver_sigpipe;
}

PipeEndpoint::PipeEndpoint()
{
	channel = NULL;
	reading = false;
}

PipeEndpoint::~PipeEndpoint()
{
	if ( channel )
		Disconnect();
}

bool PipeEndpoint::Connect(PipeEndpoint* destination)
{
	assert(!channel);
	assert(!destination->channel);
	const size_t BUFFER_SIZE = 64 * 1024;
	size_t size = BUFFER_SIZE;
	uint8_t* buffer = new uint8_t[size];
	if ( !buffer )
		return false;
	destination->reading = !(reading = false);
	if ( !(destination->channel = channel = new PipeChannel(buffer, size)) )
	{
		delete[] buffer;
		return false;
	}
	return true;
}

void PipeEndpoint::Disconnect()
{
	assert(channel);
	if ( reading )
		channel->CloseReading();
	else
		channel->CloseWriting();
	reading = false;
}

ssize_t PipeEndpoint::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	if ( !reading ) { errno = EBADF; return -1; }
	return channel->read(ctx, buf, count);
}

ssize_t PipeEndpoint::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( reading ) { errno = EBADF; return -1; }
	return channel->write(ctx, buf, count);
}

int PipeEndpoint::poll(ioctx_t* ctx, PollNode* node)
{
	return reading ? channel->read_poll(ctx, node)
	               : channel->write_poll(ctx, node);
}

bool PipeEndpoint::GetSIGPIPEDelivery()
{
	return !reading ? channel->GetSIGPIPEDelivery() : false;
}

bool PipeEndpoint::SetSIGPIPEDelivery(bool deliver_sigpipe)
{
	if ( !reading )
		channel->SetSIGPIPEDelivery(deliver_sigpipe);
	else if ( reading && deliver_sigpipe != false )
		return errno = EINVAL, false;
	return true;
}

class PipeNode : public AbstractInode
{
public:
	PipeNode(dev_t dev, uid_t owner, gid_t group, mode_t mode);
	~PipeNode();
	bool Connect(PipeNode* destination);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual int poll(ioctx_t* ctx, PollNode* node);

private:
	PipeEndpoint endpoint;

};

bool PipeNode::Connect(PipeNode* destination)
{
	return endpoint.Connect(&destination->endpoint);
}

PipeNode::PipeNode(dev_t dev, uid_t owner, gid_t group, mode_t mode)
{
	inode_type = INODE_TYPE_STREAM;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
}

PipeNode::~PipeNode()
{
}

ssize_t PipeNode::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	return endpoint.read(ctx, buf, count);
}

ssize_t PipeNode::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	return endpoint.write(ctx, buf, count);
}

int PipeNode::poll(ioctx_t* ctx, PollNode* node)
{
	return endpoint.poll(ctx, node);
}

namespace Pipe {

static int sys_pipe2(int pipefd[2], int flags)
{
	int fdflags = 0;
	if ( flags & O_CLOEXEC ) fdflags |= FD_CLOEXEC;
	if ( flags & O_CLOFORK ) fdflags |= FD_CLOFORK;
	flags &= ~(O_CLOEXEC | O_CLOFORK);

	if ( flags & ~(O_NONBLOCK) )
		return errno = EINVAL, -1;

	Process* process = CurrentProcess();
	uid_t uid = process->uid;
	uid_t gid = process->gid;
	mode_t mode = 0600;

	Ref<PipeNode> recv_inode(new PipeNode(0, uid, gid, mode));
	if ( !recv_inode ) return -1;
	Ref<PipeNode> send_inode(new PipeNode(0, uid, gid, mode));
	if ( !send_inode ) return -1;

	if ( !send_inode->Connect(recv_inode.Get()) )
		return -1;

	Ref<Vnode> recv_vnode(new Vnode(recv_inode, Ref<Vnode>(NULL), 0, 0));
	Ref<Vnode> send_vnode(new Vnode(send_inode, Ref<Vnode>(NULL), 0, 0));
	if ( !recv_vnode || !send_vnode ) return -1;

	Ref<Descriptor> recv_desc(new Descriptor(recv_vnode, O_READ));
	Ref<Descriptor> send_desc(new Descriptor(send_vnode, O_WRITE));
	if ( !recv_desc || !send_desc ) return -1;

	Ref<DescriptorTable> dtable = process->GetDTable();

	int recv_index, send_index;
	if ( 0 <= (recv_index = dtable->Allocate(recv_desc, fdflags)) )
	{
		if ( 0 <= (send_index = dtable->Allocate(send_desc, fdflags)) )
		{
			int ret[2] = { recv_index, send_index };
			if ( CopyToUser(pipefd, ret, sizeof(ret)) )
				return 0;

			dtable->Free(send_index);
		}
		dtable->Free(recv_index);
	}

	return -1;
}

void Init()
{
	Syscall::Register(SYSCALL_PIPE2, (void*) sys_pipe2);
}

} // namespace Pipe
} // namespace Sortix
