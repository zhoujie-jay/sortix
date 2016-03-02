/*
 * Copyright (c) 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * fs/user.cpp
 * User-space filesystem.
 */

#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <timespec.h>

#include <sortix/dirent.h>
#include <sortix/fcntl.h>
#include <sortix/stat.h>
#include <sortix/timespec.h>
#include <sortix/winsize.h>

#include <fsmarshall-msg.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/mtable.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>
#include <sortix/kernel/vnode.h>

namespace Sortix {
namespace UserFS {

class ChannelDirection;
class Channel;
class ChannelNode;
class Server;
class ServerNode;
class Unode;

class ChannelDirection
{
public:
	ChannelDirection();
	~ChannelDirection();
	size_t Send(ioctx_t* ctx, const void* ptr, size_t least, size_t max);
	size_t Recv(ioctx_t* ctx, void* ptr, size_t least, size_t max);
	void SendClose();
	void RecvClose();

private:
	static const size_t BUFFER_SIZE = 8192;
	uint8_t buffer[BUFFER_SIZE];
	size_t buffer_used;
	size_t buffer_offset;
	kthread_mutex_t transfer_lock;
	kthread_cond_t not_empty;
	kthread_cond_t not_full;
	bool still_reading;
	bool still_writing;

public:
	uintptr_t sender_system_tid;
	uintptr_t receiver_system_tid;

};

class Channel
{
public:
	Channel(ioctx_t* ioctx);
	~Channel();

public:
	void InformSystemTids(uintptr_t client_tid, uintptr_t server_tid);

public:
	bool KernelSend(ioctx_t* ctx, const void* ptr, size_t count)
	{
		return KernelSend(ctx, ptr, count, count) == count;
	}
	size_t KernelSend(ioctx_t* ctx, const void* ptr, size_t least, size_t max);
	bool KernelRecv(ioctx_t* ctx, void* ptr, size_t count)
	{
		return KernelRecv(ctx, ptr, count, count) == count;
	}
	size_t KernelRecv(ioctx_t* ctx, void* ptr, size_t least, size_t max);
	void KernelClose();

public:
	bool UserSend(ioctx_t* ctx, const void* ptr, size_t count)
	{
		return UserSend(ctx, ptr, count, count) == count;
	}
	size_t UserSend(ioctx_t* ctx, const void* ptr, size_t least, size_t max);
	bool UserRecv(ioctx_t* ctx, void* ptr, size_t count)
	{
		return UserRecv(ctx, ptr, count, count) == count;
	}
	size_t UserRecv(ioctx_t* ctx, void* ptr, size_t least, size_t max);
	void UserClose();

private:
	kthread_mutex_t kernel_lock;
	kthread_mutex_t user_lock;
	kthread_mutex_t destruction_lock;
	ChannelDirection from_kernel;
	ChannelDirection from_user;
	bool kernel_closed;
	bool user_closed;

public:
	uid_t uid;
	gid_t gid;

};

class ChannelNode : public AbstractInode
{
public:
	ChannelNode();
	ChannelNode(Channel* channel);
	virtual ~ChannelNode();
	void Construct(Channel* channel);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);

private:
	Channel* channel;

};

class Server : public Refcountable
{
public:
	Server();
	virtual ~Server();
	void Disconnect();
	void Unmount();
	Channel* Connect(ioctx_t* ctx);
	Channel* Accept();
	Ref<Inode> BootstrapNode(ino_t ino, mode_t type);
	Ref<Inode> OpenNode(ino_t ino, mode_t type);

private:
	kthread_mutex_t connect_lock;
	kthread_cond_t connecting_cond;
	kthread_cond_t connectable_cond;
	uintptr_t listener_system_tid;
	uintptr_t connecter_system_tid;
	Channel* connecting;
	bool disconnected;
	bool unmounted;

};

class ServerNode : public AbstractInode
{
public:
	ServerNode(Ref<Server> server);
	virtual ~ServerNode();
	virtual Ref<Inode> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
	                          int flags);

private:
	Ref<Server> server;

};

class Unode : public Inode
{
public:
	Unode(Ref<Server> server, ino_t ino, mode_t type);
	virtual ~Unode();
	virtual void linked();
	virtual void unlinked();
	virtual int sync(ioctx_t* ctx);
	virtual int stat(ioctx_t* ctx, struct stat* st);
	virtual int statvfs(ioctx_t* ctx, struct statvfs* stvfs);
	virtual int chmod(ioctx_t* ctx, mode_t mode);
	virtual int chown(ioctx_t* ctx, uid_t owner, gid_t group);
	virtual int truncate(ioctx_t* ctx, off_t length);
	virtual off_t lseek(ioctx_t* ctx, off_t offset, int whence);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t pread(ioctx_t* ctx, uint8_t* buf, size_t count,
	                      off_t off);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual ssize_t pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                       off_t off);
	virtual int utimens(ioctx_t* ctx, const struct timespec* times);
	virtual int isatty(ioctx_t* ctx);
	virtual ssize_t readdirents(ioctx_t* ctx, struct dirent* dirent,
	                            size_t size, off_t start);
	virtual Ref<Inode> open(ioctx_t* ctx, const char* filename, int flags,
	                        mode_t mode);
	virtual int mkdir(ioctx_t* ctx, const char* filename, mode_t mode);
	virtual int link(ioctx_t* ctx, const char* filename, Ref<Inode> node);
	virtual int link_raw(ioctx_t* ctx, const char* filename, Ref<Inode> node);
	virtual int unlink(ioctx_t* ctx, const char* filename);
	virtual int unlink_raw(ioctx_t* ctx, const char* filename);
	virtual int rmdir(ioctx_t* ctx, const char* filename);
	virtual int rmdir_me(ioctx_t* ctx);
	virtual int symlink(ioctx_t* ctx, const char* oldname,
	                    const char* filename);
	virtual ssize_t readlink(ioctx_t* ctx, char* buf, size_t bufsiz);
	virtual int tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp);
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	virtual int tcsetpgrp(ioctx_t* ctx, pid_t pgid);
	virtual pid_t tcgetpgrp(ioctx_t* ctx);
	virtual int settermmode(ioctx_t* ctx, unsigned mode);
	virtual int gettermmode(ioctx_t* ctx, unsigned* mode);
	virtual int poll(ioctx_t* ctx, PollNode* node);
	virtual int rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
	                        const char* newname);
	virtual Ref<Inode> accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
	                          int flags);
	virtual int bind(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	virtual int connect(ioctx_t* ctx, const uint8_t* addr, size_t addrlen);
	virtual int listen(ioctx_t* ctx, int backlog);
	virtual ssize_t recv(ioctx_t* ctx, uint8_t* buf, size_t count, int flags);
	virtual ssize_t send(ioctx_t* ctx, const uint8_t* buf, size_t count,
	                     int flags);
	virtual int getsockopt(ioctx_t* ctx, int level, int option_name,
	                       void* option_value, size_t* option_size_ptr);
	virtual int setsockopt(ioctx_t* ctx, int level, int option_name,
	                       const void* option_value, size_t option_size);
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count);
	virtual ssize_t tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count);
	virtual int unmounted(ioctx_t* ctx);
	virtual int tcdrain(ioctx_t* ctx);
	virtual int tcflow(ioctx_t* ctx, int action);
	virtual int tcflush(ioctx_t* ctx, int queue_selector);
	virtual int tcgetattr(ioctx_t* ctx, struct termios* tio);
	virtual pid_t tcgetsid(ioctx_t* ctx);
	virtual int tcsendbreak(ioctx_t* ctx, int duration);
	virtual int tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio);

private:
	bool SendMessage(Channel* channel, size_t type, void* ptr, size_t size,
	                 size_t extra = 0);
	bool RecvMessage(Channel* channel, size_t type, void* ptr, size_t size);
	void RecvError(Channel* channel);
	bool RecvBoolean(Channel* channel);
	void UnexpectedResponse(Channel* channel, struct fsm_msg_header* hdr);

private:
	ioctx_t kctx;
	Ref<Server> server;

};

//
// Implementation of Channel Directory.
//

ChannelDirection::ChannelDirection()
{
	buffer_used = 0;
	buffer_offset = 0;
	transfer_lock = KTHREAD_MUTEX_INITIALIZER;
	not_empty = KTHREAD_COND_INITIALIZER;
	not_full = KTHREAD_COND_INITIALIZER;
	still_reading = true;
	still_writing = true;
	sender_system_tid = 0;
	receiver_system_tid = 0;
}

ChannelDirection::~ChannelDirection()
{
}

size_t ChannelDirection::Send(ioctx_t* ctx, const void* ptr, size_t least, size_t max)
{
	const uint8_t* src = (const uint8_t*) ptr;
	size_t sofar = 0;
	CurrentThread()->yield_to_tid = receiver_system_tid;
	ScopedLock inner_lock(&transfer_lock);
	sender_system_tid = CurrentThread()->system_tid;
	while ( true )
	{
		while ( true )
		{
			if ( !still_reading )
				return errno = ECONNRESET, sofar;
			if ( buffer_used < BUFFER_SIZE )
				break;
			if ( least <= sofar )
				return sofar;
			if ( !kthread_cond_wait_signal(&not_full, &transfer_lock) )
				return errno = EINTR, sofar;
		}

		size_t use_offset = (buffer_offset + buffer_used) % BUFFER_SIZE;
		size_t count = max - sofar;
		size_t available_to_end = BUFFER_SIZE - use_offset;
		size_t available = BUFFER_SIZE - buffer_used;
		if ( available_to_end < available )
			available = available_to_end;
		if ( available < count )
			count = available;
		if ( !ctx->copy_from_src(buffer + use_offset, src + sofar, count) )
			return sofar;
		if ( !buffer_used )
			kthread_cond_signal(&not_empty);
		buffer_used += count;
		sofar += count;
		if ( sofar == max )
			return sofar;
	}
}

size_t ChannelDirection::Recv(ioctx_t* ctx, void* ptr, size_t least, size_t max)
{
	uint8_t* dst = (uint8_t*) ptr;
	size_t sofar = 0;
	CurrentThread()->yield_to_tid = sender_system_tid;
	ScopedLock inner_lock(&transfer_lock);
	receiver_system_tid = CurrentThread()->system_tid;
	while ( true )
	{
		while ( true )
		{
			if ( buffer_used )
				break;
			if ( least <= sofar )
				return sofar;
			if ( !still_writing )
				return errno = ECONNRESET, sofar;
			if ( !kthread_cond_wait_signal(&not_empty, &transfer_lock) )
				return errno = EINTR, sofar;
		}

		size_t use_offset = buffer_offset;
		size_t count = max - sofar;
		size_t available_to_end = BUFFER_SIZE - use_offset;
		size_t available = buffer_used;
		if ( available_to_end < available )
			available = available_to_end;
		if ( available < count )
			count = available;
		if ( !ctx->copy_to_dest(dst + sofar, buffer + use_offset, count) )
			return sofar;
		if ( buffer_used == BUFFER_SIZE )
			kthread_cond_signal(&not_full);
		buffer_offset = (buffer_offset + count) % BUFFER_SIZE;
		buffer_used -= count;
		sofar += count;
		if ( sofar == max )
			return sofar;
	}
}

void ChannelDirection::SendClose()
{
	ScopedLock lock(&transfer_lock);
	still_writing = false;
	kthread_cond_signal(&not_empty);
}

void ChannelDirection::RecvClose()
{
	ScopedLock lock(&transfer_lock);
	still_reading = false;
	kthread_cond_signal(&not_full);
}

//
// Implementation of Channel.
//

Channel::Channel(ioctx_t* ctx)
{
	kernel_lock = KTHREAD_MUTEX_INITIALIZER;
	user_lock = KTHREAD_MUTEX_INITIALIZER;
	destruction_lock = KTHREAD_MUTEX_INITIALIZER;
	user_closed = false;
	kernel_closed = false;
	uid = ctx ? ctx->uid : 0;
	gid = ctx ? ctx->gid : 0;
}

Channel::~Channel()
{
}

void Channel::InformSystemTids(uintptr_t client_tid, uintptr_t server_tid)
{
	from_kernel.sender_system_tid = client_tid;
	from_kernel.receiver_system_tid = server_tid;
	from_user.sender_system_tid = server_tid;
	from_user.receiver_system_tid = client_tid;
}

size_t Channel::KernelSend(ioctx_t* ctx, const void* ptr, size_t least,
                            size_t max)
{
	ScopedLockSignal outer_lock(&kernel_lock);
	if ( !outer_lock.IsAcquired() )
		return errno = EINTR, 0;
	size_t ret = from_kernel.Send(ctx, ptr, least, max);
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return ret;
}

size_t Channel::KernelRecv(ioctx_t* ctx, void* ptr, size_t least, size_t max)
{
	ScopedLockSignal outer_lock(&kernel_lock);
	if ( !outer_lock.IsAcquired() )
		return errno = EINTR, 0;
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return from_user.Recv(ctx, ptr, least, max);
}

void Channel::KernelClose()
{
	// No lock needed, this thread is the last to use this object as kernel.
	from_kernel.SendClose();
	from_user.RecvClose();
	kthread_mutex_lock(&destruction_lock);
	kernel_closed = true;
	bool delete_this = user_closed;
	kthread_mutex_unlock(&destruction_lock);
	if ( delete_this )
		delete this;
}

size_t Channel::UserSend(ioctx_t* ctx, const void* ptr, size_t least,
                         size_t max)
{
	ScopedLockSignal outer_lock(&user_lock);
	if ( !outer_lock.IsAcquired() )
		return errno = EINTR, 0;
	size_t ret = from_user.Send(ctx, ptr, least, max);
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return ret;
}

size_t Channel::UserRecv(ioctx_t* ctx, void* ptr, size_t least, size_t max)
{
	ScopedLockSignal outer_lock(&user_lock);
	if ( !outer_lock.IsAcquired() )
		return errno = EINTR, 0;
	size_t ret = from_kernel.Recv(ctx, ptr, least, max);
	CurrentThread()->yield_to_tid = 0;
	Scheduler::ScheduleTrueThread();
	return ret;
}

void Channel::UserClose()
{
	// No lock needed, this thread is the last to use this object as user.
	from_kernel.RecvClose();
	from_user.SendClose();
	kthread_mutex_lock(&destruction_lock);
	user_closed = true;
	bool delete_this = kernel_closed;
	kthread_mutex_unlock(&destruction_lock);
	if ( delete_this )
		delete this;
}

//
// Implementation of ChannelNode.
//

ChannelNode::ChannelNode()
{
	channel = NULL;
}

ChannelNode::ChannelNode(Channel* channel)
{
	Construct(channel);
}

ChannelNode::~ChannelNode()
{
	if ( channel )
		channel->UserClose();
}

void ChannelNode::Construct(Channel* channel)
{
	inode_type = INODE_TYPE_STREAM;
	this->channel = channel;
	this->type = S_IFCHR;
	this->dev = (dev_t) this;
	this->ino = 0;
	// TODO: Set uid, gid, mode.
}

ssize_t ChannelNode::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	return channel->UserRecv(ctx, buf, /*1*/ count, count);
}

ssize_t ChannelNode::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	return channel->UserSend(ctx, buf, /*1*/ count, count);
}

//
// Implementation of Server.
//

Server::Server()
{
	connect_lock = KTHREAD_MUTEX_INITIALIZER;
	connecting_cond = KTHREAD_COND_INITIALIZER;
	connectable_cond = KTHREAD_COND_INITIALIZER;
	listener_system_tid = 0;
	connecting = NULL;
	disconnected = false;
	unmounted = false;
}

Server::~Server()
{
}

void Server::Disconnect()
{
	ScopedLock lock(&connect_lock);
	disconnected = true;
	kthread_cond_signal(&connectable_cond);
}

void Server::Unmount()
{
	ScopedLock lock(&connect_lock);
	unmounted = true;
	kthread_cond_signal(&connecting_cond);
}

Channel* Server::Connect(ioctx_t* ctx)
{
	Channel* channel = new Channel(ctx);
	if ( !channel )
		return NULL;
	CurrentThread()->yield_to_tid = listener_system_tid;
	ScopedLock lock(&connect_lock);
	while ( !disconnected && connecting )
	{
		if ( !kthread_cond_wait_signal(&connectable_cond, &connect_lock) )
		{
			CurrentThread()->yield_to_tid = 0;
			delete channel;
			return errno = EINTR, (Channel*) NULL;
		}
	}
	CurrentThread()->yield_to_tid = 0;
	if ( disconnected )
		return delete channel, errno = ECONNREFUSED, (Channel*) NULL;
	channel->InformSystemTids(CurrentThread()->system_tid, listener_system_tid);
	connecting = channel;
	kthread_cond_signal(&connecting_cond);
	return channel;
}

Channel* Server::Accept()
{
	ScopedLock lock(&connect_lock);
	listener_system_tid = CurrentThread()->system_tid;
	while ( !connecting && !unmounted )
		if ( !kthread_cond_wait_signal(&connecting_cond, &connect_lock) )
			return errno = EINTR, (Channel*) NULL;
	if ( unmounted )
		return errno = ECONNRESET, (Channel*) NULL;
	Channel* result = connecting;
	connecting = NULL;
	kthread_cond_signal(&connectable_cond);
	return result;
}

Ref<Inode> Server::BootstrapNode(ino_t ino, mode_t type)
{
	return Ref<Inode>(new Unode(Ref<Server>(this), ino, type));
}

Ref<Inode> Server::OpenNode(ino_t ino, mode_t type)
{
	return BootstrapNode(ino, type);
}

//
// Implementation of ServerNode.
//

ServerNode::ServerNode(Ref<Server> server)
{
	inode_type = INODE_TYPE_UNKNOWN;
	this->server = server;
	this->type = S_IFSOCK;
	this->dev = (dev_t) this;
	this->ino = 0;
	// TODO: Set uid, gid, mode.
}

ServerNode::~ServerNode()
{
	server->Disconnect();
}

Ref<Inode> ServerNode::accept(ioctx_t* ctx, uint8_t* addr, size_t* addrlen,
                              int flags)
{
	(void) addr;
	(void) flags;
	size_t out_addrlen = 0;
	if ( addrlen && !ctx->copy_to_dest(addrlen, &out_addrlen, sizeof(out_addrlen)) )
		return Ref<Inode>(NULL);
	Ref<ChannelNode> node(new ChannelNode);
	if ( !node )
		return Ref<Inode>(NULL);
	Channel* channel = server->Accept();
	if ( !channel )
		return Ref<Inode>(NULL);
	node->Construct(channel);
	return node;
}

//
// Implementation of Unode.
//

Unode::Unode(Ref<Server> server, ino_t ino, mode_t type)
{
	SetupKernelIOCtx(&kctx);
	this->server = server;
	this->ino = ino;
	this->dev = (dev_t) server.Get();
	this->type = type;

	// Let the remote know that the kernel is using this inode.
	Thread* thread = CurrentThread();
	bool saved = thread->force_no_signals;
	thread->force_no_signals = true;
	thread->DoUpdatePendingSignal();
	if ( Channel* channel = server->Connect(NULL) )
	{
		struct fsm_req_refer msg;
		msg.ino = ino;
		SendMessage(channel, FSM_REQ_REFER, &msg, sizeof(msg));
		channel->KernelClose();
	}
	thread->force_no_signals = saved;
	thread->DoUpdatePendingSignal();
}

Unode::~Unode()
{
	// Let the remote know that the kernel is no longer using this inode.
	Thread* thread = CurrentThread();
	bool saved = thread->force_no_signals;
	thread->force_no_signals = true;
	thread->DoUpdatePendingSignal();
	if ( Channel* channel = server->Connect(NULL) )
	{
		struct fsm_req_unref msg;
		msg.ino = ino;
		SendMessage(channel, FSM_REQ_UNREF, &msg, sizeof(msg));
		channel->KernelClose();
	}
	thread->force_no_signals = saved;
	thread->DoUpdatePendingSignal();
}

bool Unode::SendMessage(Channel* channel, size_t type, void* ptr, size_t size,
                        size_t extra)
{
	struct fsm_msg_header hdr;
	hdr.msgtype = type;
	hdr.msgsize = size + extra;
	hdr.uid = channel->uid;
	hdr.gid = channel->gid;
	if ( !channel->KernelSend(&kctx, &hdr, sizeof(hdr)) )
		return false;
	if ( !channel->KernelSend(&kctx, ptr, size) )
		return false;
	return true;
}

bool Unode::RecvMessage(Channel* channel, size_t type, void* ptr, size_t size)
{
	struct fsm_msg_header resp_hdr;
	if ( !channel->KernelRecv(&kctx, &resp_hdr, sizeof(resp_hdr)) )
		return false;
	if ( resp_hdr.msgtype != type )
	{
		UnexpectedResponse(channel, &resp_hdr);
		return false;
	}
	return  !ptr || !size || channel->KernelRecv(&kctx, ptr, size);
}

void Unode::RecvError(Channel* channel)
{
	SetupKernelIOCtx(&kctx);
	struct fsm_resp_error resp;
	if ( channel->KernelRecv(&kctx, &resp, sizeof(resp)) )
		errno = resp.errnum;
	// In case of error, errno is set to that error.
}

bool Unode::RecvBoolean(Channel* channel)
{
	struct fsm_msg_header resp_hdr;
	if ( !channel->KernelRecv(&kctx, &resp_hdr, sizeof(resp_hdr)) )
		return false;
	if ( resp_hdr.msgtype == FSM_RESP_SUCCESS )
		return true;
	UnexpectedResponse(channel, &resp_hdr);
	return false;
}

void Unode::UnexpectedResponse(Channel* channel, struct fsm_msg_header* hdr)
{
	if ( hdr->msgtype == FSM_RESP_ERROR )
		RecvError(channel);
	else
		errno = EIO;
}

void Unode::linked()
{
}

void Unode::unlinked()
{
}

int Unode::sync(ioctx_t* ctx)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_sync msg;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_SYNC, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::stat(ioctx_t* ctx, struct stat* st)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_stat msg;
	struct fsm_resp_stat resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_STAT, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_STAT, &resp, sizeof(resp)) &&
	     (resp.st.st_dev = (dev_t) server.Get(), true) &&
	     ctx->copy_to_dest(st, &resp.st, sizeof(*st)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::statvfs(ioctx_t* ctx, struct statvfs* stvfs)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_statvfs msg;
	struct fsm_resp_statvfs resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_STATVFS, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_STATVFS, &resp, sizeof(resp)) &&
	     (resp.stvfs.f_fsid = (dev_t) server.Get(), true) &&
	     (resp.stvfs.f_flag |= ST_NOSUID, true) &&
	     ctx->copy_to_dest(stvfs, &resp.stvfs, sizeof(*stvfs)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::chmod(ioctx_t* ctx, mode_t mode)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_chmod msg;
	msg.ino = ino;
	msg.mode = mode;
	if ( SendMessage(channel, FSM_REQ_CHMOD, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::chown(ioctx_t* ctx, uid_t owner, gid_t group)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_chown msg;
	msg.ino = ino;
	msg.uid = owner;
	msg.gid = group;
	if ( SendMessage(channel, FSM_REQ_CHOWN, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::truncate(ioctx_t* ctx, off_t length)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_truncate msg;
	msg.ino = ino;
	msg.size = length;
	if ( SendMessage(channel, FSM_REQ_TRUNCATE, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

off_t Unode::lseek(ioctx_t* ctx, off_t offset, int whence)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	off_t ret = -1;
	struct fsm_req_lseek msg;
	struct fsm_resp_lseek resp;
	msg.ino = ino;
	msg.offset = offset;
	msg.whence = whence;
	if ( SendMessage(channel, FSM_REQ_LSEEK, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_LSEEK, &resp, sizeof(resp)) )
		ret = resp.offset;
	channel->KernelClose();
	return ret;
}

ssize_t Unode::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	struct fsm_req_read msg;
	struct fsm_resp_read resp;
	msg.ino = ino;
	msg.count = count;
	if ( SendMessage(channel, FSM_REQ_READ, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_READ, &resp, sizeof(resp)) )
	{
		if ( resp.count < count )
			count = resp.count;
		if ( channel->KernelRecv(ctx, buf, count) )
			ret = (ssize_t) count;
	}
	channel->KernelClose();
	return ret;
}

ssize_t Unode::pread(ioctx_t* ctx, uint8_t* buf, size_t count, off_t off)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	struct fsm_req_pread msg;
	struct fsm_resp_read resp;
	msg.ino = ino;
	msg.count = count;
	msg.offset = off;
	if ( SendMessage(channel, FSM_REQ_PREAD, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_READ, &resp, sizeof(resp)) )
	{
		if ( resp.count < count )
			count = resp.count;
		if ( channel->KernelRecv(ctx, buf, count) )
			ret = (ssize_t) count;
	}
	channel->KernelClose();
	return ret;
}

ssize_t Unode::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_write msg;
	msg.ino = ino;
	msg.count = count;
	struct fsm_msg_header hdr;
	hdr.msgtype = FSM_REQ_WRITE;
	hdr.msgsize = sizeof(msg) + count;
	hdr.uid = ctx->uid;
	hdr.gid = ctx->gid;
	struct fsm_resp_write resp;
	if ( channel->KernelSend(&kctx, &hdr, sizeof(hdr)) &&
	     channel->KernelSend(&kctx, &msg, sizeof(msg)) &&
	     channel->KernelSend(ctx, buf, count) &&
	     RecvMessage(channel, FSM_RESP_WRITE, &resp, sizeof(resp)) )
		ret = (ssize_t) resp.count;
	channel->KernelClose();
	return ret;
}

ssize_t Unode::pwrite(ioctx_t* ctx, const uint8_t* buf, size_t count, off_t off)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	struct fsm_req_pwrite msg;
	msg.ino = ino;
	msg.count = count;
	msg.offset = off;
	struct fsm_msg_header hdr;
	hdr.msgtype = FSM_REQ_PWRITE;
	hdr.msgsize = sizeof(msg) + count;
	hdr.uid = ctx->uid;
	hdr.gid = ctx->gid;
	struct fsm_resp_write resp;
	if ( channel->KernelSend(&kctx, &hdr, sizeof(hdr)) &&
	     channel->KernelSend(&kctx, &msg, sizeof(msg)) &&
	     channel->KernelSend(ctx, buf, count) &&
	     RecvMessage(channel, FSM_RESP_WRITE, &resp, sizeof(resp)) )
		ret = (ssize_t) resp.count;
	channel->KernelClose();
	return ret;
}

int Unode::utimens(ioctx_t* ctx, const struct timespec* times)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_utimens msg;
	msg.ino = ino;
	msg.times[0] = times[0];
	msg.times[1] = times[1];
	if ( SendMessage(channel, FSM_REQ_UTIMENS, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::isatty(ioctx_t* ctx)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return 0;
	int ret = 0;
	struct fsm_req_isatty msg;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_ISATTY, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 1;
	channel->KernelClose();
	return ret;
}

ssize_t Unode::readdirents(ioctx_t* ctx, struct dirent* dirent, size_t size,
                           off_t start)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	struct fsm_req_readdirents msg;
	struct fsm_resp_readdirents resp;
	msg.ino = ino;
	msg.rec_num = start;
	errno = 0;
	if ( SendMessage(channel, FSM_REQ_READDIRENTS, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_READDIRENTS, &resp, sizeof(resp)) )
	{
		if ( !resp.namelen )
		{
			ret = 0;
			goto break_if;
		}

		struct dirent entry;
		memset(&entry, 0, sizeof(entry));
		entry.d_reclen = sizeof(entry) + resp.namelen + 1;
		entry.d_namlen = resp.namelen;
		entry.d_dev = (dev_t) server.Get();
		entry.d_ino = resp.ino;
		entry.d_type = resp.type;

		if ( !ctx->copy_to_dest(dirent, &entry, sizeof(entry)) )
			goto break_if;

		if ( size < entry.d_reclen && (errno = ERANGE) )
			goto break_if;

		char nul = '\0';
		if ( channel->KernelRecv(ctx, dirent->d_name, resp.namelen) &&
		     ctx->copy_to_dest(&dirent->d_name[resp.namelen], &nul, 1) )
			ret = (ssize_t) entry.d_reclen;
	} break_if:
	channel->KernelClose();
	return ret;
}

Ref<Inode> Unode::open(ioctx_t* ctx, const char* filename, int flags,
                       mode_t mode)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return Ref<Inode>(NULL);
	size_t filenamelen = strlen(filename);
	Ref<Inode> ret;
	struct fsm_req_open msg;
	msg.dirino = ino;
	msg.flags = flags;
	msg.mode = mode;
	msg.namelen = filenamelen;
	struct fsm_resp_open resp;
	if ( SendMessage(channel, FSM_REQ_OPEN, &msg, sizeof(msg), filenamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_OPEN, &resp, sizeof(resp)) )
		ret = server->OpenNode(resp.ino, resp.type);
	channel->KernelClose();
	return ret;
}

int Unode::mkdir(ioctx_t* ctx, const char* filename, mode_t mode)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	size_t filenamelen = strlen(filename);
	int ret = -1;
	struct fsm_req_mkdir msg;
	msg.dirino = ino;
	msg.mode = mode;
	msg.namelen = filenamelen;
	struct fsm_resp_mkdir resp;
	if ( SendMessage(channel, FSM_REQ_MKDIR, &msg, sizeof(msg), filenamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_MKDIR, &resp, sizeof(resp)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::link(ioctx_t* ctx, const char* filename, Ref<Inode> node)
{
	if ( node->dev != this->dev )
		return errno = EXDEV, -1;
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	size_t filenamelen = strlen(filename);
	int ret = -1;
	struct fsm_req_link msg;
	msg.dirino = ino;
	msg.linkino = node->ino;
	msg.namelen = filenamelen;
	if ( SendMessage(channel, FSM_REQ_LINK, &msg, sizeof(msg), filenamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::link_raw(ioctx_t* /*ctx*/, const char* /*filename*/, Ref<Inode> /*node*/)
{
	return errno = EPERM, -1;
}

int Unode::unlink(ioctx_t* ctx, const char* filename)
{
	// TODO: Make sure the target is no longer used!
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	size_t filenamelen = strlen(filename);
	int ret = -1;
	struct fsm_req_unlink msg;
	msg.dirino = ino;
	msg.namelen = filenamelen;
	if ( SendMessage(channel, FSM_REQ_UNLINK, &msg, sizeof(msg), filenamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::unlink_raw(ioctx_t* /*ctx*/, const char* /*filename*/)
{
	return errno = EPERM, -1;
}

int Unode::rmdir(ioctx_t* ctx, const char* filename)
{
	// TODO: Make sure the target is no longer used!
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	size_t filenamelen = strlen(filename);
	int ret = -1;
	struct fsm_req_rmdir msg;
	msg.dirino = ino;
	msg.namelen = filenamelen;
	if ( SendMessage(channel, FSM_REQ_RMDIR, &msg, sizeof(msg), filenamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::rmdir_me(ioctx_t* /*ctx*/)
{
	return errno = EPERM, -1;
}

int Unode::symlink(ioctx_t* ctx, const char* oldname, const char* filename)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	size_t oldnamelen = strlen(oldname);
	size_t filenamelen = strlen(filename);
	int ret = -1;
	struct fsm_req_symlink msg;
	msg.dirino = ino;
	msg.targetlen = oldnamelen;
	msg.namelen = filenamelen;
	size_t extra = msg.targetlen + msg.namelen;
	if ( SendMessage(channel, FSM_REQ_SYMLINK, &msg, sizeof(msg), extra) &&
	     channel->KernelSend(&kctx, oldname, oldnamelen) &&
	     channel->KernelSend(&kctx, filename, filenamelen) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

ssize_t Unode::readlink(ioctx_t* ctx, char* buf, size_t bufsiz)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	struct fsm_req_readlink msg;
	struct fsm_resp_readlink resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_READLINK, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_READLINK, &resp, sizeof(resp)) )
	{
		if ( resp.targetlen < bufsiz )
			bufsiz = resp.targetlen;
		if ( channel->KernelRecv(ctx, buf, bufsiz) )
			ret = (ssize_t) bufsiz;
	}
	channel->KernelClose();
	return ret;
}

int Unode::tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcgetwincurpos msg;
	struct fsm_resp_tcgetwincurpos resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCGETWINCURPOS, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_TCGETWINCURPOS, &resp, sizeof(resp)) &&
	     ctx->copy_to_dest(wcp, &resp.pos, sizeof(*wcp)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcgetwinsize msg;
	struct fsm_resp_tcgetwinsize resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCGETWINSIZE, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_TCGETWINSIZE, &resp, sizeof(resp)) &&
	     ctx->copy_to_dest(ws, &resp.size, sizeof(*ws)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcsetpgrp(ioctx_t* ctx, pid_t pgid)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcsetpgrp msg;
	msg.ino = ino;
	msg.pgid = pgid;
	if ( SendMessage(channel, FSM_REQ_TCSETPGRP, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

pid_t Unode::tcgetpgrp(ioctx_t* ctx)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	pid_t ret = -1;
	struct fsm_req_tcgetpgrp msg;
	struct fsm_resp_tcgetpgrp resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCGETPGRP, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_TCGETPGRP, &resp, sizeof(resp)) )
		ret = resp.pgid;
	channel->KernelClose();
	return ret;
}

int Unode::settermmode(ioctx_t* ctx, unsigned mode)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_settermmode msg;
	msg.ino = ino;
	msg.termmode = mode;
	if ( SendMessage(channel, FSM_REQ_SETTERMMODE, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::gettermmode(ioctx_t* ctx, unsigned* mode)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_gettermmode msg;
	struct fsm_resp_gettermmode resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_GETTERMMODE, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_GETTERMMODE, &resp, sizeof(resp)) &&
	     ctx->copy_to_dest(mode, &resp.termmode, sizeof(*mode)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::poll(ioctx_t* /*ctx*/, PollNode* /*node*/)
{
	return errno = ENOTSUP, -1;
}

int Unode::rename_here(ioctx_t* ctx, Ref<Inode> from, const char* oldname,
                       const char* newname)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_rename msg;
	msg.olddirino = from->ino;
	msg.newdirino = this->ino;
	msg.oldnamelen = strlen(oldname);
	msg.newnamelen = strlen(newname);
	size_t extra = msg.oldnamelen + msg.newnamelen;
	if ( SendMessage(channel, FSM_REQ_RENAME, &msg, sizeof(msg), extra) &&
	     channel->KernelSend(&kctx, oldname, msg.oldnamelen) &&
	     channel->KernelSend(&kctx, newname, msg.newnamelen) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

Ref<Inode> Unode::accept(ioctx_t* /*ctx*/, uint8_t* /*addr*/,
                         size_t* /*addrlen*/, int /*flags*/)
{
	return errno = ENOTSOCK, Ref<Inode>();
}

int Unode::bind(ioctx_t* /*ctx*/, const uint8_t* /*addr*/, size_t /*addrlen*/)
{
	return errno = ENOTSOCK, -1;
}

int Unode::connect(ioctx_t* /*ctx*/, const uint8_t* /*addr*/, size_t /*addrlen*/)
{
	return errno = ENOTSOCK, -1;
}

int Unode::listen(ioctx_t* /*ctx*/, int /*backlog*/)
{
	return errno = ENOTSOCK, -1;
}

ssize_t Unode::recv(ioctx_t* /*ctx*/, uint8_t* /*buf*/, size_t /*count*/,
                    int /*flags*/)
{
	return errno = ENOTSOCK, -1;
}

ssize_t Unode::send(ioctx_t* /*ctx*/, const uint8_t* /*buf*/, size_t /*count*/,
                    int /*flags*/)
{
	return errno = ENOTSOCK, -1;
}

int Unode::getsockopt(ioctx_t* ctx, int level, int option_name,
                      void* option_value, size_t* option_size_ptr)
{
	size_t option_size;
	if ( !ctx->copy_from_src(&option_size, option_size_ptr, sizeof(option_size)) )
		return -1;
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_getsockopt msg;
	struct fsm_resp_getsockopt resp;
	msg.ino = ino;
	msg.level = level;
	msg.option_name = option_name;
	msg.max_option_size = option_size;
	if ( SendMessage(channel, FSM_REQ_GETSOCKOPT, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_GETSOCKOPT, &resp, sizeof(resp)) )
	{
		if ( resp.option_size < option_size )
			option_size = resp.option_size;
		if ( channel->KernelRecv(ctx, option_value, option_size) )
			ret = 0;
		if ( !ctx->copy_to_dest(option_size_ptr, &option_size, sizeof(option_size)) )
			ret = -1;
	}
	channel->KernelClose();
	return ret;
}

int Unode::setsockopt(ioctx_t* ctx, int level, int option_name,
                      const void* option_value, size_t option_size)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_setsockopt msg;
	msg.ino = ino;
	msg.level = level;
	msg.option_name = option_name;
	msg.option_size = option_size;
	if ( SendMessage(channel, FSM_REQ_SETSOCKOPT, &msg, sizeof(msg)) &&
	     channel->KernelSend(ctx, option_value, option_size) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

ssize_t Unode::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	if ( !buffer )
		count = SSIZE_MAX;
	ssize_t ret = -1;
	size_t namelen = name ? strlen(name) : 0;
	struct fsm_req_tcgetblob msg;
	struct fsm_resp_tcgetblob resp;
	msg.ino = ino;
	msg.namelen = namelen;
	if ( SendMessage(channel, FSM_REQ_TCGETBLOB, &msg, sizeof(msg), namelen) &&
	     channel->KernelSend(&kctx, name, namelen) &&
	     RecvMessage(channel, FSM_RESP_TCGETBLOB, &resp, sizeof(resp)) )
	{
		if ( resp.count < count )
			count = resp.count;
		if ( count < resp.count )
			errno = ERANGE;
		else if ( !buffer || channel->KernelRecv(ctx, buffer, count) )
			ret = (ssize_t) count;
	}
	channel->KernelClose();
	return ret;
}

ssize_t Unode::tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	ssize_t ret = -1;
	size_t namelen = name ? strlen(name) : 0;
	if ( SIZE_MAX - count < namelen )
		return errno = EOVERFLOW, -1;
	struct fsm_req_tcsetblob msg;
	struct fsm_resp_tcsetblob resp;
	msg.ino = ino;
	msg.namelen = namelen;
	msg.count = count;
	if ( SendMessage(channel, FSM_REQ_TCSETBLOB, &msg, sizeof(msg), namelen + count) &&
	     channel->KernelSend(&kctx, name, namelen) &&
	     channel->KernelSend(ctx, buffer, count) &&
	     RecvMessage(channel, FSM_RESP_TCSETBLOB, &resp, sizeof(resp)) )
		ret = (ssize_t) resp.count;
	channel->KernelClose();
	return ret;
}

int Unode::unmounted(ioctx_t* /*ctx*/)
{
	server->Unmount();
	return 0;
}

int Unode::tcdrain(ioctx_t* ctx)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcdrain msg;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCDRAIN, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcflow(ioctx_t* ctx, int action)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcflow msg;
	msg.ino = ino;
	msg.action = action;
	if ( SendMessage(channel, FSM_REQ_TCFLOW, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcflush(ioctx_t* ctx, int queue_selector)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcflush msg;
	msg.ino = ino;
	msg.queue_selector = queue_selector;
	if ( SendMessage(channel, FSM_REQ_TCFLUSH, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcgetattr(ioctx_t* ctx, struct termios* io_tio)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcgetattr msg;
	struct fsm_resp_tcgetattr resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCGETATTR, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_TCGETATTR, &msg, sizeof(msg)) &&
	     ctx->copy_to_dest(io_tio, &resp.tio, sizeof(resp.tio)) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

pid_t Unode::tcgetsid(ioctx_t* ctx)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	pid_t ret = -1;
	struct fsm_req_tcgetsid msg;
	struct fsm_resp_tcgetsid resp;
	msg.ino = ino;
	if ( SendMessage(channel, FSM_REQ_TCGETSID, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_TCGETSID, &msg, sizeof(msg)) )
		ret = resp.sid;
	channel->KernelClose();
	return ret;
}

int Unode::tcsendbreak(ioctx_t* ctx, int duration)
{
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	struct fsm_req_tcsendbreak msg;
	msg.ino = ino;
	msg.duration = duration;
	if ( SendMessage(channel, FSM_REQ_TCSENDBREAK, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

int Unode::tcsetattr(ioctx_t* ctx, int actions, const struct termios* user_tio)
{
	struct fsm_req_tcsetattr msg;
	if ( !ctx->copy_from_src(&msg.tio, user_tio, sizeof(msg.tio)) )
		return -1;
	Channel* channel = server->Connect(ctx);
	if ( !channel )
		return -1;
	int ret = -1;
	msg.ino = ino;
	msg.actions = actions;
	if ( SendMessage(channel, FSM_REQ_TCSETATTR, &msg, sizeof(msg)) &&
	     RecvMessage(channel, FSM_RESP_SUCCESS, NULL, 0) )
		ret = 0;
	channel->KernelClose();
	return ret;
}

bool Bootstrap(Ref<Inode>* out_root,
               Ref<Inode>* out_server,
               const struct stat* rootst)
{
	Ref<Server> server(new Server());
	if ( !server )
		return false;

	ino_t root_inode_ino = rootst->st_ino;
	mode_t root_inode_type = rootst->st_mode & S_IFMT;
	Ref<Inode> root_inode = server->OpenNode(root_inode_ino, root_inode_type);
	if ( !root_inode )
		return false;

	Ref<ServerNode> server_inode(new ServerNode(server));
	if ( !server_inode )
		return false;

	return *out_root = root_inode, *out_server = server_inode, true;
}

} // namespace UserFS
} // namespace Sortix
