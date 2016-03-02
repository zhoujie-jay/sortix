/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * pipe.cpp
 * A device with a writing end and a reading end.
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
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
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/vnode.h>

namespace Sortix {

class PipeChannel
{
public:
	PipeChannel(uint8_t* buffer, size_t buffersize);
	~PipeChannel();
	void CloseReading();
	void CloseWriting();
	bool GetSIGPIPEDelivery();
	void SetSIGPIPEDelivery(bool deliver_sigpipe);
	size_t ReadSize();
	size_t WriteSize();
	bool ReadResize(size_t new_size);
	bool WriteResize(size_t new_size);
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
	uintptr_t sender_system_tid;
	uintptr_t receiver_system_tid;
	size_t bufferoffset;
	size_t bufferused;
	size_t buffersize;
	size_t pretended_read_buffer_size;
	size_t pledged_read;
	size_t pledged_write;
	unsigned long closers;
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
	sender_system_tid = 0;
	receiver_system_tid = 0;
	pledged_read = 0;
	pledged_write = 0;
	closers = 0;
}

PipeChannel::~PipeChannel()
{
	delete[] buffer;
}

void PipeChannel::CloseReading()
{
	kthread_mutex_lock(&pipelock);
	anyreading = false;
	kthread_cond_broadcast(&writecond);
	read_poll_channel.Signal(ReadPollEventStatus());
	write_poll_channel.Signal(WritePollEventStatus());
	kthread_mutex_unlock(&pipelock);
	unsigned long count = InterlockedIncrement(&closers).n;
	if ( count == 2 )
		delete this;
}

void PipeChannel::CloseWriting()
{
	kthread_mutex_lock(&pipelock);
	anywriting = false;
	kthread_cond_broadcast(&readcond);
	read_poll_channel.Signal(ReadPollEventStatus());
	write_poll_channel.Signal(WritePollEventStatus());
	kthread_mutex_unlock(&pipelock);
	unsigned long count = InterlockedIncrement(&closers).n;
	if ( count == 2 )
		delete this;
}

ssize_t PipeChannel::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	Thread* this_thread = CurrentThread();
	this_thread->yield_to_tid = sender_system_tid;
	ScopedLockSignal lock(&pipelock);
	if ( !lock.IsAcquired() )
		return errno = EINTR, -1;
	size_t so_far = 0;
	while ( count )
	{
		receiver_system_tid = this_thread->system_tid;
		while ( anywriting && !bufferused )
		{
			this_thread->yield_to_tid = sender_system_tid;
			if ( pledged_read )
			{
				pledged_write++;
				kthread_mutex_unlock(&pipelock);
				kthread_yield();
				kthread_mutex_lock(&pipelock);
				pledged_write--;
				continue;
			}
			if ( so_far )
				return so_far;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			pledged_write++;
			bool interrupted = !kthread_cond_wait_signal(&readcond, &pipelock);
			pledged_write--;
			if ( interrupted )
				return errno = EINTR, -1;
		}
		if ( !bufferused && !anywriting )
			return (ssize_t) so_far;
		size_t amount = count;
		if ( bufferused < amount )
			amount = bufferused;
		size_t linear = buffersize - bufferoffset;
		if ( linear < amount )
			amount = linear;
		assert(amount);
		if ( !ctx->copy_to_dest(buf, buffer + bufferoffset, amount) )
			return so_far ? (ssize_t) so_far : -1;
		bufferoffset = (bufferoffset + amount) % buffersize;
		bufferused -= amount;
		buf += amount;
		count -= amount;
		so_far += amount;
		kthread_cond_broadcast(&writecond);
		read_poll_channel.Signal(ReadPollEventStatus());
		write_poll_channel.Signal(WritePollEventStatus());
	}
	return (ssize_t) so_far;
}

ssize_t PipeChannel::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( SSIZE_MAX < count )
		count = SSIZE_MAX;
	Thread* this_thread = CurrentThread();
	this_thread->yield_to_tid = receiver_system_tid;
	ScopedLockSignal lock(&pipelock);
	if ( !lock.IsAcquired() )
		return errno = EINTR, -1;
	sender_system_tid = this_thread->system_tid;
	size_t so_far = 0;
	while ( count )
	{
		sender_system_tid = this_thread->system_tid;
		while ( anyreading && bufferused == buffersize )
		{
			this_thread->yield_to_tid = receiver_system_tid;
			if ( pledged_write )
			{
				pledged_read++;
				kthread_mutex_unlock(&pipelock);
				kthread_yield();
				kthread_mutex_lock(&pipelock);
				pledged_read--;
				continue;
			}
			if ( so_far )
				return so_far;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			pledged_read++;
			bool interrupted = !kthread_cond_wait_signal(&writecond, &pipelock);
			pledged_read--;
			if ( interrupted )
				return errno = EINTR, -1;
		}
		if ( !anyreading )
		{
			if ( so_far )
				return (ssize_t) so_far;
			if ( is_sigpipe_enabled )
				CurrentThread()->DeliverSignal(SIGPIPE);
			return errno = EPIPE, -1;
		}
		size_t amount = count;
		if ( buffersize - bufferused < amount )
			amount = buffersize - bufferused;
		size_t writeoffset = (bufferoffset + bufferused) % buffersize;
		size_t linear = buffersize - writeoffset;
		if ( linear < amount )
			amount = linear;
		assert(amount);
		if ( !ctx->copy_from_src(buffer + writeoffset, buf, amount) )
			return so_far ? (ssize_t) so_far : -1;
		bufferused += amount;
		buf += amount;
		count -= amount;
		so_far += amount;
		kthread_cond_broadcast(&readcond);
		read_poll_channel.Signal(ReadPollEventStatus());
		write_poll_channel.Signal(WritePollEventStatus());
	}
	return (ssize_t) so_far;
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

size_t PipeChannel::ReadSize()
{
	ScopedLockSignal lock(&pipelock);
	return pretended_read_buffer_size;
}

size_t PipeChannel::WriteSize()
{
	ScopedLockSignal lock(&pipelock);
	return buffersize;
}

bool PipeChannel::ReadResize(size_t new_size)
{
	ScopedLockSignal lock(&pipelock);
	if ( !new_size )
		return errno = EINVAL, false;
	// The read and write end share the same buffer, so let the write end decide
	// how big a buffer it wants and pretend the read end can decide too.
	pretended_read_buffer_size = new_size;
	return true;
}

bool PipeChannel::WriteResize(size_t new_size)
{
	ScopedLockSignal lock(&pipelock);
	if ( !new_size )
		return errno = EINVAL, false;

	size_t MAX_PIPE_SIZE = 2 * 1024 * 1024;
	if ( MAX_PIPE_SIZE < new_size )
		new_size = MAX_PIPE_SIZE;

	// Refuse to lose data if the the new size would cause truncation.
	if ( new_size < bufferused )
		new_size = bufferused;

	uint8_t* new_buffer = new uint8_t[new_size];
	if ( !new_buffer )
		return false;

	for ( size_t i = 0; i < bufferused; i++ )
		new_buffer[i] = buffer[(bufferoffset + i) % buffersize];
	delete[] buffer;
	buffer = new_buffer;
	buffersize = new_size;

	return true;
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
	channel = NULL;
	reading = false;
}

ssize_t PipeEndpoint::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	if ( !reading )
		return errno = EBADF, -1;
	ssize_t result = channel->read(ctx, buf, count);
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return result;
}

ssize_t PipeEndpoint::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	if ( reading )
		return errno = EBADF, -1;
	ssize_t result = channel->write(ctx, buf, count);
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return result;
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

size_t PipeEndpoint::Size()
{
	return reading ? channel->ReadSize()
	               : channel->WriteSize();
}

bool PipeEndpoint::Resize(size_t new_size)
{
	return reading ? channel->ReadResize(new_size)
	               : channel->WriteResize(new_size);
}

class PipeNode : public AbstractInode
{
public:
	PipeNode(dev_t dev, uid_t owner, gid_t group, mode_t mode);
	virtual ~PipeNode();
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual int poll(ioctx_t* ctx, PollNode* node);

public:
	bool Connect(PipeNode* destination);

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

int sys_pipe2(int* pipefd, int flags)
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

} // namespace Sortix
