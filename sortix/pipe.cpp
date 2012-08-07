/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/copy.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/vnode.h>
#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/dtable.h>
#include <sortix/signal.h>
#include <sortix/stat.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "signal.h"
#include "thread.h"
#include "process.h"
#include "syscall.h"
#include "pipe.h"

namespace Sortix {

class PipeChannel
{
public:
	PipeChannel(uint8_t* buffer, size_t buffersize);
	~PipeChannel();
	void StartReading();
	void StartWriting();
	void CloseReading();
	void CloseWriting();
	void PerhapsShutdown();
	ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);

private:
	kthread_mutex_t pipelock;
	kthread_cond_t readcond;
	kthread_cond_t writecond;
	uint8_t* buffer;
	size_t bufferoffset;
	size_t bufferused;
	size_t buffersize;
	bool anyreading;
	bool anywriting;

};

PipeChannel::PipeChannel(uint8_t* buffer, size_t buffersize)
{
	pipelock = KTHREAD_MUTEX_INITIALIZER;
	readcond = KTHREAD_COND_INITIALIZER;
	writecond = KTHREAD_COND_INITIALIZER;
	this->buffer = buffer;
	this->buffersize = buffersize;
	bufferoffset = bufferused = 0;
	anyreading = anywriting = false;
}

PipeChannel::~PipeChannel()
{
	delete[] buffer;
}

void PipeChannel::StartReading()
{
	ScopedLock lock(&pipelock);
	assert(!anyreading);
	anyreading = true;
}

void PipeChannel::StartWriting()
{
	ScopedLock lock(&pipelock);
	assert(!anywriting);
	anywriting = true;
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
	return amount;
}

ssize_t PipeChannel::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	ScopedLockSignal lock(&pipelock);
	if ( !lock.IsAcquired() ) { errno = EINTR; return -1; }
	while ( anyreading && bufferused == buffersize )
	{
		if ( !kthread_cond_wait_signal(&writecond, &pipelock) )
		{
			errno = EINTR;
			return -1;
		}
	}
	if ( !anyreading )
	{
		CurrentThread()->DeliverSignal(SIGPIPE);
		errno = EPIPE;
		return -1;
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
	return amount;
}

class PipeEndpoint : public AbstractInode
{
public:
	PipeEndpoint(dev_t dev, uid_t owner, gid_t group, mode_t mode,
	             PipeChannel* channel, bool reading);
	~PipeEndpoint();
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);

private:
	kthread_mutex_t pipelock;
	PipeChannel* channel;
	bool reading;

};

PipeEndpoint::PipeEndpoint(dev_t dev, uid_t owner, gid_t group, mode_t mode,
                           PipeChannel* channel, bool reading)
{
	inode_type = INODE_TYPE_STREAM;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->channel = channel;
	this->reading = reading;
	if ( reading )
		channel->StartReading();
	else
		channel->StartWriting();
	pipelock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
}

PipeEndpoint::~PipeEndpoint()
{
	if ( reading )
		channel->CloseReading();
	else
		channel->CloseWriting();
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

namespace Pipe {

const size_t BUFFER_SIZE = 4096UL;

static int sys_pipe(int pipefd[2])
{
	Process* process = CurrentProcess();
	uid_t uid = process->uid;
	uid_t gid = process->gid;
	mode_t mode = 0600;

	size_t buffersize = BUFFER_SIZE;
	uint8_t* buffer = new uint8_t[buffersize];
	if ( !buffer ) return -1;

	PipeChannel* channel = new PipeChannel(buffer, buffersize);
	if ( !channel ) { delete[] buffer; return -1; }

	Ref<Inode> recv_inode(new PipeEndpoint(0, uid, gid, mode, channel, true));
	if ( !recv_inode ) { delete channel; return -1; }
	Ref<Inode> send_inode(new PipeEndpoint(0, uid, gid, mode, channel, false));
	if ( !send_inode ) return -1;

	Ref<Vnode> recv_vnode(new Vnode(recv_inode, Ref<Vnode>(NULL), 0, 0));
	Ref<Vnode> send_vnode(new Vnode(send_inode, Ref<Vnode>(NULL), 0, 0));
	if ( !recv_vnode || !send_vnode ) return -1;

	Ref<Descriptor> recv_desc(new Descriptor(recv_vnode, 0));
	Ref<Descriptor> send_desc(new Descriptor(send_vnode, 0));
	if ( !recv_desc || !send_desc ) return -1;

	Ref<DescriptorTable> dtable = process->GetDTable();

	int recv_index, send_index;
	if ( 0 <= (recv_index = dtable->Allocate(recv_desc, 0)) )
	{
		if ( 0 <= (send_index = dtable->Allocate(send_desc, 0)) )
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
	Syscall::Register(SYSCALL_PIPE, (void*) sys_pipe);
}

} // namespace Pipe
} // namespace Sortix
